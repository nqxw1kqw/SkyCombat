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

// Loads the first path that exists; game assets are looked up relative to
// the working directory (exe folder / repo root / build folder).
ObjMesh loadFirstMesh(std::initializer_list<const char*> paths, double targetLength) {
    ObjMesh mesh;
    for (const char* path : paths) {
        if (mesh.load(path, targetLength)) break;
    }
    return mesh;
}

const ObjMesh& playerMesh() {
    static const ObjMesh mesh = loadFirstMesh(
        {"assets/models/fighter_jet.obj", "../assets/models/fighter_jet.obj",
         "../../assets/models/fighter_jet.obj"},
        15.0);
    return mesh;
}

const ObjMesh& enemyMesh() {
    static const ObjMesh mesh = loadFirstMesh(
        {"assets/studio/models/enemy_fighter_jet.obj", "../assets/studio/models/enemy_fighter_jet.obj",
         "../../assets/studio/models/enemy_fighter_jet.obj"},
        15.0);
    return mesh;
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

} // namespace
#endif


void AircraftRenderer::draw(const Aircraft& a) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    // Local nose +Z; roll is purely visual and cannot steer flight or camera.
    const Quat rotation = a.transform.orientation * Quat::axisAngle({0, 0, 1}, a.visualBank);
    // Real model first (Shin's fighter_jet.obj); primitive jet is the fallback.
    if (!playerMesh().empty()) { drawMesh(playerMesh(), a, rotation); return; }
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
    const bool drewMesh = !enemyMesh().empty();
    if (drewMesh) drawMesh(enemyMesh(), a, rotation);
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
