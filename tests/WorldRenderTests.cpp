#include "Test.h"
#include "world/SkySystem.h"
#include "world/BiomeSystem.h"
#include "world/TerrainSystem.h"
#include "world/TerrainChunk.h"
#include <cmath>
#include <vector>

using namespace sky;

int main() {
    Tests t;

    // =========================================================================
    // 1. SkySystem Tests
    // =========================================================================
    SkySystem sky;
    t.check(near(sky.evaluateFogFactor(200.0), 0.0), "fog factor is 0 within near distance");
    t.check(near(sky.evaluateFogFactor(4000.0), 1.0), "fog factor is 1 at or beyond far distance");
    t.check(sky.evaluateFogFactor(1000.0) < sky.evaluateFogFactor(2000.0), "fog factor is strictly monotonic with distance");

    Vec3 zenithCol = sky.evaluateSkyColor({0, 1, 0});
    Vec3 horizonCol = sky.evaluateSkyColor({1, 0, 0});
    t.check(near(length(zenithCol - sky.colors().zenith), 0.0), "sky color looking straight up matches zenith color");
    t.check(near(length(horizonCol - sky.colors().horizon), 0.0), "sky color looking at horizon matches horizon color");
    t.check(near(length(sky.fog().color - sky.colors().horizon), 0.0), "fog color matches horizon for seamless fade");

    // =========================================================================
    // 2. Biome Determinism & Weight Normalization
    // =========================================================================
    BiomeSystem biomes;
    Vec3 nUp{0, 1, 0};
    Vec3 col1 = biomes.evaluateColor(-400.0, 100.0, 35.0, nUp);
    Vec3 col2 = biomes.evaluateColor(-400.0, 100.0, 35.0, nUp);
    t.check(near(length(col1 - col2), 0.0), "biome color evaluation is deterministic");

    bool weightsNormalized = true;
    for (double x = -800; x <= 800; x += 200) {
        for (double z = -800; z <= 800; z += 200) {
            auto w = biomes.evaluateWeights(x, z, 40.0, nUp);
            double sum = w.grassland + w.purpleFlower + w.pinkFlower + w.highland + w.mountainRock + w.valleyFloor;
            if (std::abs(sum - 1.0) > 1e-6) weightsNormalized = false;
        }
    }
    t.check(weightsNormalized, "biome weights strictly sum to 1.0 across all coordinates");

    // =========================================================================
    // 3. Biome Layout Sanity (Section 6 & Reference Colors)
    // =========================================================================
    // Center airfield
    auto centerBiome = biomes.classifyBiome(0.0, 0.0, 18.0, nUp);
    t.check(centerBiome == BiomeType::Grassland, "center airfield classifies as Grassland");

    // West flower field
    auto westBiome = biomes.classifyBiome(-550.0, 50.0, 35.0, nUp);
    t.check(westBiome == BiomeType::PurpleFlowerField, "west mid-latitude classifies as PurpleFlowerField");
    Vec3 westCol = biomes.evaluateColor(-550.0, 50.0, 35.0, nUp);
    t.check(westCol.z > westCol.x && westCol.z > westCol.y, "west flower color is predominant in blue/violet channel");

    // East flower field
    auto eastBiome = biomes.classifyBiome(550.0, 50.0, 35.0, nUp);
    t.check(eastBiome == BiomeType::PinkFlowerField, "east mid-latitude classifies as PinkFlowerField");
    Vec3 eastCol = biomes.evaluateColor(550.0, 50.0, 35.0, nUp);
    t.check(eastCol.x > eastCol.y && eastCol.x > eastCol.z, "east flower color is predominant in red/pink channel");

    // High North Mountains
    auto northMountainBiome = biomes.classifyBiome(0.0, 750.0, 160.0, nUp);
    t.check(northMountainBiome == BiomeType::MountainRock, "high north mountain classifies as MountainRock");

    // Steep cliff at any altitude
    Vec3 cliffNormal = normalized(Vec3{0.8, 0.2, 0.0}); // very steep slope
    auto cliffBiome = biomes.classifyBiome(0.0, 0.0, 25.0, cliffNormal);
    t.check(cliffBiome == BiomeType::MountainRock, "steep cliff face classifies as MountainRock");

    // South canyon floor
    double vz = -450.0;
    double vx = 180.0 * std::sin(vz * 0.006) + 70.0 * std::cos(vz * 0.014);
    auto valleyBiome = biomes.classifyBiome(vx, vz, 6.0, nUp);
    t.check(valleyBiome == BiomeType::ValleyFloor, "south canyon floor classifies as ValleyFloor");

    // =========================================================================
    // 4. Border Blending Continuity (Smooth C^1 transitions, no hard seams)
    // =========================================================================
    double maxStepDelta = 0.0;
    Vec3 prevCol = biomes.evaluateColor(-100.0, 50.0, 35.0, nUp);
    for (double x = -105.0; x >= -450.0; x -= 5.0) {
        Vec3 currCol = biomes.evaluateColor(x, 50.0, 35.0, nUp);
        double delta = length(currCol - prevCol);
        if (delta > maxStepDelta) maxStepDelta = delta;
        prevCol = currCol;
    }
    t.check(maxStepDelta < 0.06, "color transition across West flower border is smooth and continuous without seams");

    // =========================================================================
    // 5. Terrain Chunk Coverage (16 chunks covering 2x2 km)
    // =========================================================================
    TerrainSystem terrain(WorldConfig::fixedSeed);
    terrain.initialize();
    t.check(terrain.chunkCount() == 16, "terrain contains exactly 16 chunks (4x4)");

    bool allCovered = true;
    for (double x = -950; x <= 950; x += 100) {
        for (double z = -950; z <= 950; z += 100) {
            const auto* c = terrain.findChunk(x, z);
            if (!c || x < c->minX() || x > c->maxX() || z < c->minZ() || z > c->maxZ()) {
                allCovered = false;
            }
        }
    }
    t.check(allCovered, "16 chunks fully cover the 2x2 km world without gaps");

    // =========================================================================
    // 6. Chunk Border Height Parity at Equal LOD (No cracks)
    // =========================================================================
    // Test horizontal border between Chunk (0, 0) and Chunk (1, 0)
    const auto* c00 = terrain.findChunk(-750, -750); // chunkX = 0, chunkZ = 0
    const auto* c10 = terrain.findChunk(-250, -750); // chunkX = 1, chunkZ = 0
    t.check(c00 && c10, "adjacent chunks found");

    for (int lod = 0; lod < TerrainChunk::numLODs; ++lod) {
        const auto& m0 = c00->mesh(lod);
        const auto& m1 = c10->mesh(lod);

        // Find vertices on the shared border x = -500.0
        std::vector<double> y0, y1;
        for (const auto& v : m0.vertices) {
            if (near(v.position.x, -500.0, 1e-4)) y0.push_back(v.position.y);
        }
        for (const auto& v : m1.vertices) {
            if (near(v.position.x, -500.0, 1e-4)) y1.push_back(v.position.y);
        }

        bool matchLOD = (y0.size() == y1.size() && !y0.empty());
        if (matchLOD) {
            for (size_t i = 0; i < y0.size(); ++i) {
                if (std::abs(y0[i] - y1[i]) > 1e-10) matchLOD = false;
            }
        }
        t.check(matchLOD, "shared chunk border vertices have identical heights at equal LOD (no cracks)");
    }

    // =========================================================================
    // 7. Distance-based LOD Selection & Hysteresis Stability
    // =========================================================================
    TerrainChunk testChunk(0, 0, 0.0, 0.0); // [0, 500] x [0, 500]
    testChunk.forceLOD(0);

    t.check(testChunk.updateLOD(300.0) == 0, "close distance selects LOD 0");
    t.check(testChunk.updateLOD(600.0) == 0, "distance 600m retains LOD 0 (below 650m switch threshold)");
    t.check(testChunk.updateLOD(700.0) == 1, "distance 700m transitions to LOD 1 (>650m)");
    t.check(testChunk.updateLOD(600.0) == 1, "hysteresis: distance 600m retains LOD 1 (above 550m return threshold)");
    t.check(testChunk.updateLOD(500.0) == 0, "distance 500m transitions back to LOD 0 (<550m)");

    testChunk.forceLOD(1);
    t.check(testChunk.updateLOD(1250.0) == 2, "distance 1250m transitions to LOD 2 (>1200m)");
    t.check(testChunk.updateLOD(1150.0) == 2, "hysteresis: distance 1150m retains LOD 2 (above 1100m return threshold)");
    t.check(testChunk.updateLOD(1050.0) == 1, "distance 1050m transitions back to LOD 1 (<1100m)");

    // =========================================================================
    // 8. Distant Mountain Silhouette Proxy
    // =========================================================================
    const auto& proxy = terrain.distantMountains();
    t.check(!proxy.triangles.empty() && proxy.vertices.size() >= 8, "distant mountain proxy geometry generated");
    bool allNorth = true;
    for (const auto& v : proxy.vertices) {
        if (v.position.z < 1000.0) allNorth = false;
    }
    t.check(allNorth, "distant mountain proxy is placed beyond northern boundary (z > 1000m)");

    return t.finish();
}
