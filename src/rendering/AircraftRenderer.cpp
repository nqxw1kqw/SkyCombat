#include "AircraftRenderer.h"
#include "ObjMesh.h"
#include "combat/Bullet.h"
#include "combat/Missile.h"

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace sky {

#if !defined(SKY_PORTABLE) && defined(_WIN32)
namespace {

// One aircraft's visual set: body mesh plus its authored attachments.
// The plume and wind shells are modelled in the body's own local frame,
// so they are loaded raw and given the body's normalisation transform.
// Game assets are looked up relative to the working directory
// (exe folder / repo root / build folder).
struct JetVisuals {
    ObjMesh jet, plume, wind;
    double tailZ = -7.5; // normalised tail plane (min z of the body)
};

bool loadFirstRaw(ObjMesh& mesh, std::initializer_list<const char*> paths) {
    for (const char* path : paths) {
        if (mesh.loadRaw(path)) return true;
    }
    return false;
}

JetVisuals loadVisuals(std::initializer_list<const char*> jetPaths,
                       std::initializer_list<const char*> plumePaths,
                       std::initializer_list<const char*> windPaths) {
    JetVisuals vis;
    if (!loadFirstRaw(vis.jet, jetPaths)) return vis;
    Vec3 mn, mx;
    vis.jet.bounds(mn, mx);
    const Vec3 center = (mn + mx) * 0.5;
    const double extent = std::max({mx.x - mn.x, mx.y - mn.y, mx.z - mn.z});
    const double scale = extent > 1e-9 ? 15.0 / extent : 1.0;
    vis.jet.applyTransform(center, scale);
    vis.tailZ = (mn.z - center.z) * scale;
    if (loadFirstRaw(vis.plume, plumePaths)) vis.plume.applyTransform(center, scale);
    if (loadFirstRaw(vis.wind, windPaths)) vis.wind.applyTransform(center, scale);
    return vis;
}

const JetVisuals& playerVisuals() {
    static const JetVisuals vis = loadVisuals(
        {"assets/models/fighter_jet.obj", "../assets/models/fighter_jet.obj",
         "../../assets/models/fighter_jet.obj"},
        {"assets/studio/vfx/player_thruster_plume.obj", "../assets/studio/vfx/player_thruster_plume.obj",
         "../../assets/studio/vfx/player_thruster_plume.obj"},
        {"assets/studio/vfx/player_wind_airflow_vfx.obj", "../assets/studio/vfx/player_wind_airflow_vfx.obj",
         "../../assets/studio/vfx/player_wind_airflow_vfx.obj"});
    return vis;
}

const JetVisuals& enemyVisuals() {
    static const JetVisuals vis = loadVisuals(
        {"assets/studio/models/enemy_fighter_jet.obj", "../assets/studio/models/enemy_fighter_jet.obj",
         "../../assets/studio/models/enemy_fighter_jet.obj"},
        {"assets/studio/vfx/enemy_thruster_plume.obj", "../assets/studio/vfx/enemy_thruster_plume.obj",
         "../../assets/studio/vfx/enemy_thruster_plume.obj"},
        {"assets/studio/vfx/enemy_wind_airflow_vfx.obj", "../assets/studio/vfx/enemy_wind_airflow_vfx.obj",
         "../../assets/studio/vfx/enemy_wind_airflow_vfx.obj"});
    return vis;
}

unsigned int toDxColor(unsigned int packed) {
    return GetColor(int((packed >> 16) & 0xFF), int((packed >> 8) & 0xFF), int(packed & 0xFF));
}

unsigned int shadeColor(unsigned int color, double shade) {
    const int r = std::min(255, int(double(int((color >> 16) & 0xFF)) * shade + 0.5));
    const int g = std::min(255, int(double(int((color >> 8) & 0xFF)) * shade + 0.5));
    const int b = std::min(255, int(double(int(color & 0xFF)) * shade + 0.5));
    return GetColor(r, g, b);
}

// Draws an OBJ mesh in aircraft space with a simple two-sided flat
// diffuse shade so the fuselage reads as a solid 3D shape.
void drawMesh(const ObjMesh& mesh, const Aircraft& a, const Quat& rotation) {
    static const Vec3 sunDir = normalized(Vec3{0.4, 0.8, 0.3});
    for (const auto& tri : mesh.triangles()) {
        VECTOR v[3];
        Vec3 w[3];
        for (int i = 0; i < 3; ++i) {
            w[i] = a.transform.position + rotation.rotate(tri.p[i]);
            v[i] = VGet(float(w[i].x), float(w[i].y), float(w[i].z));
        }
        const Vec3 n = normalized(cross(w[1] - w[0], w[2] - w[0]));
        const double shade = 0.70 + 0.30 * std::fabs(dot(n, sunDir));
        DrawTriangle3D(v[0], v[1], v[2], shadeColor(tri.color, shade), TRUE);
    }
}

// Exhaust plume: additive glow, no depth write. Vertices behind the tail
// plane stretch along -Z as speed rises, with a fast shimmer on top.
void drawPlume(const JetVisuals& vis, const Aircraft& a, const Quat& rotation) {
    if (vis.plume.empty()) return;
    const double speedNorm = std::clamp((a.speed - 25.0) / (140.0 - 25.0), 0.0, 1.0);
    const double t = GetNowCount() * 0.001;
    const double flicker = 1.0 + 0.06 * std::sin(t * 47.0) + 0.04 * std::sin(t * 31.0 + 1.7);
    const double lenScale = (0.55 + 0.75 * speedNorm) * flicker;
    SetDrawBlendMode(DX_BLENDMODE_ADD, 205);
    SetWriteZBuffer3D(FALSE);
    for (const auto& tri : vis.plume.triangles()) {
        VECTOR v[3];
        for (int i = 0; i < 3; ++i) {
            Vec3 p = tri.p[i];
            if (p.z < vis.tailZ) p.z = vis.tailZ + (p.z - vis.tailZ) * lenScale;
            const Vec3 w = a.transform.position + rotation.rotate(p);
            v[i] = VGet(float(w.x), float(w.y), float(w.z));
        }
        DrawTriangle3D(v[0], v[1], v[2], toDxColor(tri.color), TRUE);
    }
    SetWriteZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// Supersonic airflow shell (vapor cone, slipstream ribbons, wingtip
// vortices): fades in only at high speed, from 100 m/s up.
void drawWind(const JetVisuals& vis, const Aircraft& a, const Quat& rotation) {
    if (vis.wind.empty()) return;
    const double intensity = std::clamp((a.speed - 100.0) / 30.0, 0.0, 1.0);
    if (intensity <= 0.0) return;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, int(40 + 110 * intensity));
    SetWriteZBuffer3D(FALSE);
    for (const auto& tri : vis.wind.triangles()) {
        VECTOR v[3];
        for (int i = 0; i < 3; ++i) {
            const Vec3 w = a.transform.position + rotation.rotate(tri.p[i]);
            v[i] = VGet(float(w.x), float(w.y), float(w.z));
        }
        DrawTriangle3D(v[0], v[1], v[2], toDxColor(tri.color), TRUE);
    }
    SetWriteZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void drawEffects(const JetVisuals& vis, const Aircraft& a, const Quat& rotation) {
    drawPlume(vis, a, rotation);
    drawWind(vis, a, rotation);
}

} // namespace
#endif


void AircraftRenderer::draw(const Aircraft& a) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    // Local nose +Z; roll is purely visual and cannot steer flight or camera.
    const Quat rotation = a.transform.orientation * Quat::axisAngle({0, 0, 1}, a.visualBank);
    // Real model first (Shin's fighter_jet.obj); primitive jet is the fallback.
    const JetVisuals& vis = playerVisuals();
    if (!vis.jet.empty()) { drawMesh(vis.jet, a, rotation); drawEffects(vis, a, rotation); return; }
    const std::array<Vec3, 10> points{{{0, 0, 7}, {-1, 0, -4}, {1, 0, -4}, {0, 1.3, -1},
        {0, -0.8, -1}, {-6, 0, -2}, {6, 0, -2}, {-2.8, 0, -4.5}, {2.8, 0, -4.5}, {0, 2.8, -4}}};
    std::array<VECTOR, 10> v{};
    for (size_t i = 0; i < v.size(); ++i) {
        auto p = a.transform.position + rotation.rotate(points[i]);
        v[i] = VGet(float(p.x), float(p.y), float(p.z));
    }
    struct Face { int a, b, c; unsigned int color; };
    const auto body = GetColor(225, 232, 239), shade = GetColor(117, 143, 166), wing = GetColor(239, 163, 65);
    const Face faces[] = {{0, 1, 3, body}, {0, 3, 2, body}, {0, 4, 1, shade}, {0, 2, 4, shade},
                          {1, 2, 3, shade}, {1, 4, 2, shade}, {0, 5, 1, wing}, {0, 2, 6, wing},
                          {1, 7, 3, wing}, {2, 3, 8, wing}, {1, 9, 2, body}};
    for (auto f : faces) DrawTriangle3D(v[f.a], v[f.b], v[f.c], f.color, TRUE);
#else
    (void)a;
#endif
}

void AircraftRenderer::drawEnemy(const Aircraft& a, bool isTargeted, bool isLocked) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    if (a.health <= 0.0 || a.crashed) return;

    const Quat rotation = a.transform.orientation * Quat::axisAngle({0, 0, 1}, a.visualBank);
    // Real model first (studio enemy_fighter_jet.obj); primitive jet is the fallback.
    const JetVisuals& vis = enemyVisuals();
    const bool drewMesh = !vis.jet.empty();
    if (drewMesh) { drawMesh(vis.jet, a, rotation); drawEffects(vis, a, rotation); }
    const std::array<Vec3, 10> points{{{0, 0, 7}, {-1, 0, -4}, {1, 0, -4}, {0, 1.3, -1},
        {0, -0.8, -1}, {-6, 0, -2}, {6, 0, -2}, {-2.8, 0, -4.5}, {2.8, 0, -4.5}, {0, 2.8, -4}}};
    std::array<VECTOR, 10> v{};
    for (size_t i = 0; i < v.size(); ++i) {
        auto p = a.transform.position + rotation.rotate(points[i]);
        v[i] = VGet(float(p.x), float(p.y), float(p.z));
    }
    struct Face { int a, b, c; unsigned int color; };
    // Hostile enemy palette: charcoal body with crimson wings
    const auto body = GetColor(60, 65, 75), shade = GetColor(40, 45, 52), wing = GetColor(220, 50, 50);
    const Face faces[] = {{0, 1, 3, body}, {0, 3, 2, body}, {0, 4, 1, shade}, {0, 2, 4, shade},
                          {1, 2, 3, shade}, {1, 4, 2, shade}, {0, 5, 1, wing}, {0, 2, 6, wing},
                          {1, 7, 3, wing}, {2, 3, 8, wing}, {1, 9, 2, body}};
    if (!drewMesh) {
        for (auto f : faces) DrawTriangle3D(v[f.a], v[f.b], v[f.c], f.color, TRUE);
    }

    // Target reticle
    if (isLocked) {
        VECTOR p = VGet(float(a.transform.position.x), float(a.transform.position.y), float(a.transform.position.z));
        DrawSphere3D(p, 5.0f, 8, GetColor(255, 220, 50), GetColor(255, 220, 50), FALSE);
    } else if (isTargeted) {
        VECTOR p = VGet(float(a.transform.position.x), float(a.transform.position.y), float(a.transform.position.z));
        DrawSphere3D(p, 4.5f, 6, GetColor(80, 200, 240), GetColor(80, 200, 240), FALSE);
    }
#else
    (void)a; (void)isTargeted; (void)isLocked;
#endif
}

void AircraftRenderer::drawBullet(const Bullet& b) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    if (!b.active) return;
    Vec3 p0 = b.position;
    Vec3 p1 = b.position - normalized(b.velocity) * 3.5;
    uint32_t col = b.fromPlayer ? GetColor(255, 240, 100) : GetColor(255, 90, 80);
    DrawLine3D(VGet(float(p0.x), float(p0.y), float(p0.z)),
               VGet(float(p1.x), float(p1.y), float(p1.z)),
               col);
#else
    (void)b;
#endif
}

void AircraftRenderer::drawMissile(const Missile& m) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    if (!m.isActive()) return;
    Vec3 p0 = m.position();
    Vec3 p1 = m.position() - m.forward() * 4.0;
    DrawLine3D(VGet(float(p0.x), float(p0.y), float(p0.z)),
               VGet(float(p1.x), float(p1.y), float(p1.z)),
               GetColor(255, 255, 255));

    // Rocket plume (studio prefab): authored at missile scale with its
    // base at the origin and the flame along -Z, so it only needs a basis
    // built from the missile's forward vector. Additive, no depth write.
    static const ObjMesh missilePlume = [] {
        ObjMesh mesh;
        loadFirstRaw(mesh, {"assets/studio/vfx/missile_thruster_vfx_prefab.obj",
                            "../assets/studio/vfx/missile_thruster_vfx_prefab.obj",
                            "../../assets/studio/vfx/missile_thruster_vfx_prefab.obj"});
        return mesh;
    }();
    if (!missilePlume.empty()) {
        const Vec3 f = normalized(m.forward());
        Vec3 r = cross(Vec3{0, 1, 0}, f);
        r = length(r) < 1e-6 ? Vec3{1, 0, 0} : normalized(r);
        const Vec3 u = cross(f, r);
        const double t = GetNowCount() * 0.001;
        const double flicker = 1.0 + 0.08 * std::sin(t * 53.0) + 0.05 * std::sin(t * 37.0 + 0.6);
        const Vec3 base = m.position() - f * 1.5; // anchor mid-body, flame trails behind
        SetDrawBlendMode(DX_BLENDMODE_ADD, 205);
        SetWriteZBuffer3D(FALSE);
        for (const auto& tri : missilePlume.triangles()) {
            VECTOR v[3];
            for (int i = 0; i < 3; ++i) {
                const Vec3 p = tri.p[i];
                const Vec3 w = base + r * p.x + u * p.y + f * (p.z * 1.1 * flicker);
                v[i] = VGet(float(w.x), float(w.y), float(w.z));
            }
            DrawTriangle3D(v[0], v[1], v[2], toDxColor(tri.color), TRUE);
        }
        SetWriteZBuffer3D(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // Smoke trail
    const auto& tr = m.trail();
    for (size_t i = 1; i < tr.size(); ++i) {
        DrawLine3D(VGet(float(tr[i - 1].x), float(tr[i - 1].y), float(tr[i - 1].z)),
                   VGet(float(tr[i].x), float(tr[i].y), float(tr[i].z)),
                   GetColor(200, 210, 225));
    }
#else
    (void)m;
#endif
}

} // namespace sky
