#include "TerrainSystem.h"
#include <cmath>
#include <algorithm>

namespace sky {

namespace {

inline uint32_t hash2D(int x, int z, uint32_t seed) {
    uint32_t h = seed ^ (uint32_t(x) * 374761393u) ^ (uint32_t(z) * 668265263u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

inline double smoothstep(double e0, double e1, double x) {
    if (e0 == e1) return x < e0 ? 0.0 : 1.0;
    double t = std::clamp((x - e0) / (e1 - e0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

inline double gradNoise(double x, double z, uint32_t seed) {
    int x0 = int(std::floor(x));
    int z0 = int(std::floor(z));
    double fx = x - x0;
    double fz = z - z0;

    // Quintic Hermite curve for smooth derivative
    double u = fx * fx * fx * (fx * (fx * 6.0 - 15.0) + 10.0);
    double v = fz * fz * fz * (fz * (fz * 6.0 - 15.0) + 10.0);

    static constexpr double gx[8] = { 1, -1,  0,  0,  0.70710678, -0.70710678,  0.70710678, -0.70710678 };
    static constexpr double gz[8] = { 0,  0,  1, -1,  0.70710678,  0.70710678, -0.70710678, -0.70710678 };

    auto dotGrad = [&](int ix, int iz, double rx, double rz) {
        uint32_t h = hash2D(ix, iz, seed) & 7;
        return gx[h] * rx + gz[h] * rz;
    };

    double d00 = dotGrad(x0,     z0,     fx,       fz);
    double d10 = dotGrad(x0 + 1, z0,     fx - 1.0, fz);
    double d01 = dotGrad(x0,     z0 + 1, fx,       fz - 1.0);
    double d11 = dotGrad(x0 + 1, z0 + 1, fx - 1.0, fz - 1.0);

    double xTop = d00 + u * (d10 - d00);
    double xBot = d01 + u * (d11 - d01);
    return xTop + v * (xBot - xTop);
}

inline double fbm(double x, double z, int octaves, double lacunarity, double gain, uint32_t seed) {
    double sum = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double maxAmp = 0.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * gradNoise(x * freq, z * freq, seed + uint32_t(i * 31));
        maxAmp += amp;
        freq *= lacunarity;
        amp *= gain;
    }
    return maxAmp > 0.0 ? (sum / maxAmp) : 0.0;
}

inline double ridgedNoise(double x, double z, int octaves, uint32_t seed) {
    double sum = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double weight = 1.0;
    for (int i = 0; i < octaves; ++i) {
        double n = 1.0 - std::abs(gradNoise(x * freq, z * freq, seed + uint32_t(i * 47)));
        n = n * n;
        sum += n * amp * weight;
        weight = std::clamp(n * 2.0, 0.0, 1.0);
        freq *= 2.0;
        amp *= 0.5;
    }
    return sum;
}

} // namespace

TerrainSystem::TerrainSystem(uint32_t seed) : seed_(seed) {}

double TerrainSystem::evaluateHills(double x, double z, uint32_t seed) {
    double base = 18.0;
    double noise = fbm(x * 0.003, z * 0.003, 3, 2.0, 0.5, seed);
    double distFromCenter = std::sqrt(x * x + z * z);
    double centerFlattener = smoothstep(60.0, 300.0, distFromCenter);
    return base + noise * 18.0 * (0.2 + 0.8 * centerFlattener);
}

double TerrainSystem::evaluateMountainMask(double x, double z) {
    // North begins transitioning at z = 80, fully mountainous by z = 450
    double northFactor = smoothstep(80.0, 450.0, z);
    // Smooth boundary falloff near edge limits
    double edgeTaper = smoothstep(1000.0, 850.0, std::abs(x)) * smoothstep(1000.0, 850.0, z);
    return northFactor * edgeTaper;
}

double TerrainSystem::evaluateMountains(double x, double z, uint32_t seed) {
    double ridge = ridgedNoise(x * 0.0025, z * 0.0025, 4, seed + 101);
    return ridge * 160.0; // peak elevation component
}

double TerrainSystem::evaluateDetail(double x, double z, uint32_t seed) {
    return fbm(x * 0.025, z * 0.025, 2, 2.0, 0.5, seed + 202) * 3.0;
}

double TerrainSystem::evaluateValleyMask(double x, double z) {
    // South winding valley begins transitioning south of z = -50
    double southFactor = smoothstep(-50.0, -250.0, z);
    // Winding valley path along south axis
    double valleyX = 180.0 * std::sin(z * 0.006) + 70.0 * std::cos(z * 0.014);
    double distValley = std::abs(x - valleyX);
    double valleyCross = smoothstep(120.0, 30.0, distValley);
    double edgeTaperSouth = smoothstep(1000.0, 850.0, std::abs(x)) * smoothstep(-1000.0, -850.0, z);
    return southFactor * valleyCross * edgeTaperSouth;
}

double TerrainSystem::evaluateValleyDepth(double x, double z, uint32_t seed) {
    double depthNoise = 0.8 + 0.4 * fbm(x * 0.005, z * 0.005, 2, 2.0, 0.5, seed + 303);
    return 26.0 * depthNoise;
}

double TerrainSystem::evaluateAnalyticHeight(double x, double z, uint32_t seed) {
    double hills = evaluateHills(x, z, seed);
    double mountainMask = evaluateMountainMask(x, z);
    double mountains = evaluateMountains(x, z, seed);
    double detail = evaluateDetail(x, z, seed);
    double valleyMask = evaluateValleyMask(x, z);
    double valleyDepth = evaluateValleyDepth(x, z, seed);

    double h = hills + mountainMask * mountains + detail - valleyMask * valleyDepth;
    return std::max(h, 2.0); // Safe floor above zero level
}

void TerrainSystem::buildDistantMountains() {
    distantMountains_.vertices.clear();
    distantMountains_.triangles.clear();
    distantMountains_.indices.clear();

    // Generate distant northern mountain silhouette ridge beyond z = 1000
    // Simple low-poly proxy ridge with 9 peaks across x in [-1200, 1200]
    struct Peak { double x, z, y; };
    const Peak peaks[] = {
        {-1200.0, 1400.0, 180.0},
        {-900.0,  1550.0, 270.0},
        {-600.0,  1450.0, 230.0},
        {-300.0,  1650.0, 320.0},
        {   0.0,  1500.0, 280.0},
        { 300.0,  1700.0, 350.0},
        { 600.0,  1550.0, 290.0},
        { 900.0,  1650.0, 260.0},
        {1200.0,  1400.0, 190.0}
    };
    constexpr size_t numPeaks = sizeof(peaks) / sizeof(peaks[0]);

    // Slate-blue atmospheric mountain rock color
    const Vec3 baseRock{0.50, 0.55, 0.62};
    const Vec3 lightDir = normalized({0.4, 0.8, 0.3});

    auto packColor = [&](const Vec3& normal) -> uint32_t {
        double diff = 0.6 + 0.4 * std::max(0.0, dot(normal, lightDir));
        int r = std::clamp(int(baseRock.x * diff * 255.0 + 0.5), 0, 255);
        int g = std::clamp(int(baseRock.y * diff * 255.0 + 0.5), 0, 255);
        int b = std::clamp(int(baseRock.z * diff * 255.0 + 0.5), 0, 255);
        return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
    };

    // Base vertices along z = 1050 (near boundary) and peak vertices
    for (size_t i = 0; i < numPeaks - 1; ++i) {
        Vec3 b0{peaks[i].x, 60.0, 1050.0};
        Vec3 p0{peaks[i].x, peaks[i].y, peaks[i].z};
        Vec3 p1{peaks[i + 1].x, peaks[i + 1].y, peaks[i + 1].z};
        Vec3 b1{peaks[i + 1].x, 60.0, 1050.0};

        uint32_t idx = uint32_t(distantMountains_.vertices.size());
        distantMountains_.vertices.push_back({b0, {0, 1, 0}, 0});
        distantMountains_.vertices.push_back({p0, {0, 1, 0}, 0});
        distantMountains_.vertices.push_back({p1, {0, 1, 0}, 0});
        distantMountains_.vertices.push_back({b1, {0, 1, 0}, 0});

        // Face 1: (b0, p0, p1)
        Vec3 n1 = normalized(cross(p0 - b0, p1 - b0));
        uint32_t col1 = packColor(n1);
        distantMountains_.triangles.push_back({idx, idx + 1, idx + 2, n1, col1});
        distantMountains_.indices.push_back(idx);
        distantMountains_.indices.push_back(idx + 1);
        distantMountains_.indices.push_back(idx + 2);

        // Face 2: (b0, p1, b1)
        Vec3 n2 = normalized(cross(p1 - b0, b1 - b0));
        uint32_t col2 = packColor(n2);
        distantMountains_.triangles.push_back({idx, idx + 2, idx + 3, n2, col2});
        distantMountains_.indices.push_back(idx);
        distantMountains_.indices.push_back(idx + 2);
        distantMountains_.indices.push_back(idx + 3);
    }
}

void TerrainSystem::initialize() {
    heights_.resize(WorldConfig::numVertsX * WorldConfig::numVertsZ);
    mesh_.vertices.clear();
    mesh_.indices.clear();
    mesh_.triangles.clear();
    mesh_.vertices.reserve(WorldConfig::numVertsX * WorldConfig::numVertsZ);

    for (int iz = 0; iz < WorldConfig::numVertsZ; ++iz) {
        double z = WorldConfig::minZ + iz * WorldConfig::cellSize;
        for (int ix = 0; ix < WorldConfig::numVertsX; ++ix) {
            double x = WorldConfig::minX + ix * WorldConfig::cellSize;
            double y = evaluateAnalyticHeight(x, z, seed_);
            heights_[iz * WorldConfig::numVertsX + ix] = y;

            TerrainVertex vert;
            vert.position = {x, y, z};
            vert.normal = {0, 1, 0};
            mesh_.vertices.push_back(vert);
        }
    }

    // Generate base mesh triangles using BiomeSystem colors
    mesh_.triangles.reserve(WorldConfig::numCellsX * WorldConfig::numCellsZ * 2);
    mesh_.indices.reserve(WorldConfig::numCellsX * WorldConfig::numCellsZ * 6);

    for (int iz = 0; iz < WorldConfig::numCellsZ; ++iz) {
        for (int ix = 0; ix < WorldConfig::numCellsX; ++ix) {
            uint32_t v00 = uint32_t(iz * WorldConfig::numVertsX + ix);
            uint32_t v10 = uint32_t(iz * WorldConfig::numVertsX + (ix + 1));
            uint32_t v01 = uint32_t((iz + 1) * WorldConfig::numVertsX + ix);
            uint32_t v11 = uint32_t((iz + 1) * WorldConfig::numVertsX + (ix + 1));

            // Triangle 0: (v00, v10, v11) covering fx >= fz
            {
                Vec3 p0 = mesh_.vertices[v00].position;
                Vec3 p1 = mesh_.vertices[v10].position;
                Vec3 p2 = mesh_.vertices[v11].position;
                Vec3 n = normalized(cross(p1 - p0, p2 - p0));
                Vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
                uint32_t col = biomes_.evaluatePackedColor(center.x, center.z, center.y, n);

                mesh_.triangles.push_back({v00, v10, v11, n, col});
                mesh_.indices.push_back(v00);
                mesh_.indices.push_back(v10);
                mesh_.indices.push_back(v11);
            }

            // Triangle 1: (v00, v11, v01) covering fx < fz
            {
                Vec3 p0 = mesh_.vertices[v00].position;
                Vec3 p1 = mesh_.vertices[v11].position;
                Vec3 p2 = mesh_.vertices[v01].position;
                Vec3 n = normalized(cross(p1 - p0, p2 - p0));
                Vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
                uint32_t col = biomes_.evaluatePackedColor(center.x, center.z, center.y, n);

                mesh_.triangles.push_back({v00, v11, v01, n, col});
                mesh_.indices.push_back(v00);
                mesh_.indices.push_back(v11);
                mesh_.indices.push_back(v01);
            }
        }
    }

    // Build 16 terrain chunks (4x4, 500m x 500m each) with 3 LOD levels each
    chunks_.clear();
    chunks_.reserve(16);
    for (int cz = 0; cz < 4; ++cz) {
        double minZ = WorldConfig::minZ + cz * TerrainChunk::chunkSize;
        for (int cx = 0; cx < 4; ++cx) {
            double minX = WorldConfig::minX + cx * TerrainChunk::chunkSize;
            chunks_.emplace_back(cx, cz, minX, minZ);
        }
    }

    for (auto& chunk : chunks_) {
        chunk.buildMeshes(*this, biomes_, seed_);
    }

    // Build distant mountain silhouettes
    buildDistantMountains();

    initialized_ = true;
}

double TerrainSystem::groundHeight(double x, double z) const {
    if (!initialized_) {
        return evaluateAnalyticHeight(x, z, seed_);
    }

    // Clamp coordinates to world boundaries
    double cx = std::clamp(x, WorldConfig::minX, WorldConfig::maxX);
    double cz = std::clamp(z, WorldConfig::minZ, WorldConfig::maxZ);

    double u = (cx - WorldConfig::minX) / WorldConfig::cellSize;
    double v = (cz - WorldConfig::minZ) / WorldConfig::cellSize;

    int ix = int(std::floor(u));
    int iz = int(std::floor(v));
    if (ix < 0) ix = 0;
    if (ix >= WorldConfig::numCellsX) ix = WorldConfig::numCellsX - 1;
    if (iz < 0) iz = 0;
    if (iz >= WorldConfig::numCellsZ) iz = WorldConfig::numCellsZ - 1;

    double fx = std::clamp(u - ix, 0.0, 1.0);
    double fz = std::clamp(v - iz, 0.0, 1.0);

    double h00 = heights_[iz * WorldConfig::numVertsX + ix];
    double h10 = heights_[iz * WorldConfig::numVertsX + (ix + 1)];
    double h01 = heights_[(iz + 1) * WorldConfig::numVertsX + ix];
    double h11 = heights_[(iz + 1) * WorldConfig::numVertsX + (ix + 1)];

    // Exact triangle-consistent plane interpolation
    // Triangle 0 (v00, v10, v11) for fx >= fz
    // Triangle 1 (v00, v11, v01) for fx < fz
    if (fx >= fz) {
        return h00 + fx * (h10 - h00) + fz * (h11 - h10);
    } else {
        return h00 + fx * (h11 - h01) + fz * (h01 - h00);
    }
}

void TerrainSystem::updateLODs(const Vec3& cameraPosition) {
    for (auto& chunk : chunks_) {
        double d = chunk.distanceTo(cameraPosition);
        chunk.updateLOD(d);
    }
}

const TerrainChunk* TerrainSystem::findChunk(double x, double z) const {
    if (x < WorldConfig::minX || x > WorldConfig::maxX ||
        z < WorldConfig::minZ || z > WorldConfig::maxZ) {
        return nullptr;
    }
    int cx = int((x - WorldConfig::minX) / TerrainChunk::chunkSize);
    int cz = int((z - WorldConfig::minZ) / TerrainChunk::chunkSize);
    cx = std::clamp(cx, 0, 3);
    cz = std::clamp(cz, 0, 3);
    size_t idx = size_t(cz * 4 + cx);
    return idx < chunks_.size() ? &chunks_[idx] : nullptr;
}

} // namespace sky
