#include "VegetationSystem.h"
#include "TerrainSystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

namespace {

// Deterministic 32-bit linear congruential generator
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

} // namespace

VegetationSystem::VegetationSystem(uint32_t seed) : seed_(seed) {}

void VegetationSystem::addCrossedQuad(TerrainMeshData& batch, const Vec3& base, double width, double height,
                                     uint32_t baseColor, uint32_t tipColor) {
    uint32_t idx = uint32_t(batch.vertices.size());
    double hw = width * 0.5;

    // Quad 1: along X-axis
    Vec3 p0{base.x - hw, base.y, base.z};
    Vec3 p1{base.x + hw, base.y, base.z};
    Vec3 p2{base.x + hw, base.y + height, base.z};
    Vec3 p3{base.x - hw, base.y + height, base.z};

    // Quad 2: along Z-axis
    Vec3 p4{base.x, base.y, base.z - hw};
    Vec3 p5{base.x, base.y, base.z + hw};
    Vec3 p6{base.x, base.y + height, base.z + hw};
    Vec3 p7{base.x, base.y + height, base.z - hw};

    batch.vertices.push_back({p0, {0, 0, 1}, baseColor});
    batch.vertices.push_back({p1, {0, 0, 1}, baseColor});
    batch.vertices.push_back({p2, {0, 0, 1}, tipColor});
    batch.vertices.push_back({p3, {0, 0, 1}, tipColor});

    batch.vertices.push_back({p4, {1, 0, 0}, baseColor});
    batch.vertices.push_back({p5, {1, 0, 0}, baseColor});
    batch.vertices.push_back({p6, {1, 0, 0}, tipColor});
    batch.vertices.push_back({p7, {1, 0, 0}, tipColor});

    // Quad 1 triangles
    batch.triangles.push_back({idx + 0, idx + 1, idx + 2, {0, 0, 1}, tipColor});
    batch.triangles.push_back({idx + 0, idx + 2, idx + 3, {0, 0, 1}, tipColor});
    batch.indices.push_back(idx + 0); batch.indices.push_back(idx + 1); batch.indices.push_back(idx + 2);
    batch.indices.push_back(idx + 0); batch.indices.push_back(idx + 2); batch.indices.push_back(idx + 3);

    // Quad 2 triangles
    batch.triangles.push_back({idx + 4, idx + 5, idx + 6, {1, 0, 0}, tipColor});
    batch.triangles.push_back({idx + 4, idx + 6, idx + 7, {1, 0, 0}, tipColor});
    batch.indices.push_back(idx + 4); batch.indices.push_back(idx + 5); batch.indices.push_back(idx + 6);
    batch.indices.push_back(idx + 4); batch.indices.push_back(idx + 6); batch.indices.push_back(idx + 7);
}

void VegetationSystem::build(const IHeightProvider& heightProvider, const BiomeSystem& biomes) {
    grassBatch_.vertices.clear();
    grassBatch_.triangles.clear();
    grassBatch_.indices.clear();

    purpleFlowersBatch_.vertices.clear();
    purpleFlowersBatch_.triangles.clear();
    purpleFlowersBatch_.indices.clear();

    pinkFlowersBatch_.vertices.clear();
    pinkFlowersBatch_.triangles.clear();
    pinkFlowersBatch_.indices.clear();

    grassPositions_.clear();
    purpleFlowerPositions_.clear();
    pinkFlowerPositions_.clear();

    DeterministicRng rng(seed_);

    // Helper lambda to check shared terrain exclusion rules
    auto isTerrainValid = [&](double x, double z, double y, const Vec3& normal, const BiomeWeights& weights) -> bool {
        // 1. Slope exclusion: steep slopes cannot support vegetation
        if (normal.y < 0.85) return false;
        // 2. High altitude and mountain rock exclusion
        if (y > 85.0 || weights.mountainRock > 0.05) return false;
        // 3. Valley floor canyon path exclusion
        double valleyMask = TerrainSystem::evaluateValleyMask(x, z);
        if (weights.valleyFloor > 0.05 || valleyMask > 0.05) return false;
        return true;
    };

    // 1. Scatter grass clumps across the central and highland terrain
    constexpr int maxGrassCandidates = 5000;
    for (int i = 0; i < maxGrassCandidates && grassPositions_.size() < size_t(WorldConfig::maxGrassClumps); ++i) {
        double x = rng.nextDouble(-800.0, 800.0);
        double z = rng.nextDouble(-800.0, 800.0);
        double y = heightProvider.groundHeight(x, z);

        double dx = heightProvider.groundHeight(x + 1.0, z) - heightProvider.groundHeight(x - 1.0, z);
        double dz = heightProvider.groundHeight(x, z + 1.0) - heightProvider.groundHeight(x, z - 1.0);
        Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});

        auto weights = biomes.evaluateWeights(x, z, y, normal);
        if (!isTerrainValid(x, z, y, normal, weights)) continue;

        auto biome = biomes.classifyBiome(x, z, y, normal);
        if (biome == BiomeType::Grassland || biome == BiomeType::Highland) {
            double width = rng.nextDouble(1.1, 1.6);
            double height = rng.nextDouble(0.9, 1.4);
            addCrossedQuad(grassBatch_, {x, y, z}, width, height, 0x003e6b22, 0x0082c35f);
            grassPositions_.push_back({x, y, z});
        }
    }

    // 2. Scatter purple flower tufts strictly in Western purple flower field (X < 0)
    constexpr int maxPurpleCandidates = 2000;
    for (int i = 0; i < maxPurpleCandidates && purpleFlowerPositions_.size() < size_t(WorldConfig::maxPurpleFlowerTufts); ++i) {
        double x = rng.nextDouble(-650.0, -130.0);
        double z = rng.nextDouble(-500.0, 350.0);
        double y = heightProvider.groundHeight(x, z);

        double dx = heightProvider.groundHeight(x + 1.0, z) - heightProvider.groundHeight(x - 1.0, z);
        double dz = heightProvider.groundHeight(x, z + 1.0) - heightProvider.groundHeight(x, z - 1.0);
        Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});

        auto weights = biomes.evaluateWeights(x, z, y, normal);
        if (!isTerrainValid(x, z, y, normal, weights)) continue;

        auto biome = biomes.classifyBiome(x, z, y, normal);
        if (biome == BiomeType::PurpleFlowerField && x < 0.0 && weights.purpleFlower > 0.30) {
            double width = rng.nextDouble(1.1, 1.5);
            double height = rng.nextDouble(0.9, 1.3);
            addCrossedQuad(purpleFlowersBatch_, {x, y, z}, width, height, 0x00478028, 0x00bc94e2);
            purpleFlowerPositions_.push_back({x, y, z});
        }
    }

    // 3. Scatter pink flower tufts strictly in Eastern pink flower field (X > 0)
    constexpr int maxPinkCandidates = 2000;
    for (int i = 0; i < maxPinkCandidates && pinkFlowerPositions_.size() < size_t(WorldConfig::maxPinkFlowerTufts); ++i) {
        double x = rng.nextDouble(130.0, 650.0);
        double z = rng.nextDouble(-500.0, 350.0);
        double y = heightProvider.groundHeight(x, z);

        double dx = heightProvider.groundHeight(x + 1.0, z) - heightProvider.groundHeight(x - 1.0, z);
        double dz = heightProvider.groundHeight(x, z + 1.0) - heightProvider.groundHeight(x, z - 1.0);
        Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});

        auto weights = biomes.evaluateWeights(x, z, y, normal);
        if (!isTerrainValid(x, z, y, normal, weights)) continue;

        auto biome = biomes.classifyBiome(x, z, y, normal);
        if (biome == BiomeType::PinkFlowerField && x > 0.0 && weights.pinkFlower > 0.30) {
            double width = rng.nextDouble(1.1, 1.5);
            double height = rng.nextDouble(0.9, 1.3);
            addCrossedQuad(pinkFlowersBatch_, {x, y, z}, width, height, 0x00478028, 0x00edb5c9);
            pinkFlowerPositions_.push_back({x, y, z});
        }
    }
}

} // namespace sky
