#pragma once
#include "HeightProvider.h"
#include "TerrainMesh.h"
#include "TerrainChunk.h"
#include "BiomeSystem.h"
#include <vector>
#include <cstdint>

namespace sky {

class TerrainSystem : public IHeightProvider {
public:
    explicit TerrainSystem(uint32_t seed = WorldConfig::fixedSeed);

    // Analytic height function: H(x,z) = Hills + MountainMask*Mountains + Detail - ValleyMask*ValleyDepth
    static double evaluateAnalyticHeight(double x, double z, uint32_t seed = WorldConfig::fixedSeed);

    // Component functions for inspection, tests and design tuning
    static double evaluateHills(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateMountainMask(double x, double z);
    static double evaluateMountains(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateDetail(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateValleyMask(double x, double z);
    static double evaluateValleyDepth(double x, double z, uint32_t seed = WorldConfig::fixedSeed);

    // Initializes grid data, biome colors, chunks, and distant mountains
    void initialize();

    // IHeightProvider: triangle-consistent ground height matching mesh exactly
    double groundHeight(double x, double z) const override;

    // Updates LOD for all chunks based on camera position
    void updateLODs(const Vec3& cameraPosition);

    uint32_t seed() const { return seed_; }
    bool initialized() const { return initialized_; }
    const TerrainMeshData& mesh() const { return mesh_; }
    const std::vector<double>& heights() const { return heights_; }
    const BiomeSystem& biomes() const { return biomes_; }

    // Chunks (4x4 = 16 chunks covering the 2x2 km world)
    const std::vector<TerrainChunk>& chunks() const { return chunks_; }
    std::vector<TerrainChunk>& chunks() { return chunks_; }
    int chunkCount() const { return int(chunks_.size()); }
    const TerrainChunk* findChunk(double x, double z) const;

    // Distant northern mountain proxy mesh
    const TerrainMeshData& distantMountains() const { return distantMountains_; }

private:
    void buildDistantMountains();

    uint32_t seed_{WorldConfig::fixedSeed};
    std::vector<double> heights_;
    TerrainMeshData mesh_;
    BiomeSystem biomes_;
    std::vector<TerrainChunk> chunks_;
    TerrainMeshData distantMountains_;
    bool initialized_{false};
};

} // namespace sky
