#include "CloudSystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

namespace {

struct DeterministicRng {
    uint32_t state;
    explicit DeterministicRng(uint32_t seed) : state(seed == 0 ? 1337u : seed) {}

    uint32_t next() {
        state = state * 1664525u + 1013904223u;
        return state;
    }

    double nextDouble(double minVal, double maxVal) {
        double t = double(next() >> 8) / double(1u << 24);
        return minVal + t * (maxVal - minVal);
    }
};

void addBox(TerrainMeshData& mesh, const Vec3& center, const Vec3& halfSize,
            uint32_t topColor, uint32_t bottomColor, uint32_t sideColor) {
    uint32_t baseIdx = uint32_t(mesh.vertices.size());
    double hx = halfSize.x, hy = halfSize.y, hz = halfSize.z;

    // 8 box corners
    const Vec3 p[8] = {
        {center.x - hx, center.y - hy, center.z - hz}, // 0
        {center.x + hx, center.y - hy, center.z - hz}, // 1
        {center.x + hx, center.y - hy, center.z + hz}, // 2
        {center.x - hx, center.y - hy, center.z + hz}, // 3
        {center.x - hx, center.y + hy, center.z - hz}, // 4
        {center.x + hx, center.y + hy, center.z - hz}, // 5
        {center.x + hx, center.y + hy, center.z + hz}, // 6
        {center.x - hx, center.y + hy, center.z + hz}  // 7
    };

    for (int i = 0; i < 8; ++i) {
        uint32_t col = (i >= 4) ? topColor : bottomColor;
        mesh.vertices.push_back({p[i], {0, 1, 0}, col});
    }

    auto addFace = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t d, const Vec3& n, uint32_t col) {
        mesh.triangles.push_back({baseIdx + a, baseIdx + b, baseIdx + c, n, col});
        mesh.triangles.push_back({baseIdx + a, baseIdx + c, baseIdx + d, n, col});
        mesh.indices.push_back(baseIdx + a); mesh.indices.push_back(baseIdx + b); mesh.indices.push_back(baseIdx + c);
        mesh.indices.push_back(baseIdx + a); mesh.indices.push_back(baseIdx + c); mesh.indices.push_back(baseIdx + d);
    };

    // Top face
    addFace(4, 5, 6, 7, {0, 1, 0}, topColor);
    // Bottom face
    addFace(0, 3, 2, 1, {0, -1, 0}, bottomColor);
    // 4 Sides
    addFace(0, 1, 5, 4, {0, 0, -1}, sideColor);
    addFace(1, 2, 6, 5, {1, 0, 0}, sideColor);
    addFace(2, 3, 7, 6, {0, 0, 1}, sideColor);
    addFace(3, 0, 4, 7, {-1, 0, 0}, sideColor);
}

} // namespace

CloudSystem::CloudSystem(uint32_t seed) : seed_(seed) {}

void CloudSystem::initialize() {
    clusters_.clear();
    clusters_.reserve(WorldConfig::numCloudClusters);

    DeterministicRng rng(seed_ + 999);

    for (int i = 0; i < WorldConfig::numCloudClusters; ++i) {
        CloudCluster cl;
        cl.position.x = rng.nextDouble(-1150.0, 1150.0);
        cl.position.z = rng.nextDouble(-1150.0, 1150.0);
        cl.position.y = rng.nextDouble(minAltitude_, maxAltitude_);

        cl.size.x = rng.nextDouble(65.0, 110.0);
        cl.size.y = rng.nextDouble(14.0, 24.0);
        cl.size.z = rng.nextDouble(75.0, 130.0);
        cl.boxCount = int(rng.nextDouble(2.0, 4.5));

        clusters_.push_back(cl);
    }

    rebuildMesh();
}

void CloudSystem::rebuildMesh() {
    mesh_.vertices.clear();
    mesh_.triangles.clear();
    mesh_.indices.clear();
    clusterRanges_.clear();
    clusterRanges_.reserve(clusters_.size());

    const uint32_t topCol = 0x00f8fbff;    // Puffy white
    const uint32_t bottomCol = 0x00c4d2e2; // Soft shaded underside
    const uint32_t sideCol = 0x00e2ebf5;   // Soft rim

    DeterministicRng rng(seed_ + 555);

    for (const auto& cl : clusters_) {
        size_t startVert = mesh_.vertices.size();

        // Base center blob
        Vec3 halfBase{cl.size.x * 0.5, cl.size.y * 0.5, cl.size.z * 0.5};
        addBox(mesh_, cl.position, halfBase, topCol, bottomCol, sideCol);

        // Sub-blobs overlapping around the cluster center
        for (int b = 0; b < cl.boxCount; ++b) {
            double ox = rng.nextDouble(-cl.size.x * 0.35, cl.size.x * 0.35);
            double oy = rng.nextDouble(-cl.size.y * 0.2, cl.size.y * 0.2);
            double oz = rng.nextDouble(-cl.size.z * 0.35, cl.size.z * 0.35);

            double sx = cl.size.x * rng.nextDouble(0.3, 0.45);
            double sy = cl.size.y * rng.nextDouble(0.35, 0.45);
            double sz = cl.size.z * rng.nextDouble(0.3, 0.45);

            Vec3 subCenter{cl.position.x + ox, cl.position.y + oy, cl.position.z + oz};
            addBox(mesh_, subCenter, {sx, sy, sz}, topCol, bottomCol, sideCol);
        }

        size_t vertCount = mesh_.vertices.size() - startVert;
        clusterRanges_.push_back({startVert, vertCount});
    }
}

void CloudSystem::update(double dt) {
    if (dt <= 0.0) return;
    double deltaX = driftSpeed_ * dt;

    for (size_t i = 0; i < clusters_.size(); ++i) {
        auto& cl = clusters_[i];
        double oldX = cl.position.x;
        cl.position.x += deltaX;
        double actualDeltaX = deltaX;
        if (cl.position.x > 1200.0) {
            cl.position.x -= 2400.0;
            actualDeltaX = cl.position.x - oldX;
        }

        const auto& range = clusterRanges_[i];
        for (size_t v = range.startVertex; v < range.startVertex + range.vertexCount; ++v) {
            mesh_.vertices[v].position.x += actualDeltaX;
        }
    }
}

} // namespace sky
