#pragma once
#include "TerrainMesh.h"
#include "HeightProvider.h"
#include "BiomeSystem.h"
#include "WorldConfig.h"
#include <vector>
#include <cstdint>

namespace sky {

class VegetationSystem {
public:
    explicit VegetationSystem(uint32_t seed = WorldConfig::fixedSeed);

    // Deterministically generates vegetation batches
    void build(const IHeightProvider& heightProvider, const BiomeSystem& biomes);

    const TerrainMeshData& grassBatch() const { return grassBatch_; }
    const TerrainMeshData& purpleFlowersBatch() const { return purpleFlowersBatch_; }
    const TerrainMeshData& pinkFlowersBatch() const { return pinkFlowersBatch_; }

    size_t grassClumpCount() const { return grassPositions_.size(); }
    size_t purpleFlowerCount() const { return purpleFlowerPositions_.size(); }
    size_t pinkFlowerCount() const { return pinkFlowerPositions_.size(); }

    const std::vector<Vec3>& grassPositions() const { return grassPositions_; }
    const std::vector<Vec3>& purpleFlowerPositions() const { return purpleFlowerPositions_; }
    const std::vector<Vec3>& pinkFlowerPositions() const { return pinkFlowerPositions_; }

    uint32_t seed() const { return seed_; }

private:
    void addCrossedQuad(TerrainMeshData& batch, const Vec3& base, double width, double height,
                        uint32_t baseColor, uint32_t tipColor);

    uint32_t seed_{WorldConfig::fixedSeed};
    TerrainMeshData grassBatch_;
    TerrainMeshData purpleFlowersBatch_;
    TerrainMeshData pinkFlowersBatch_;

    std::vector<Vec3> grassPositions_;
    std::vector<Vec3> purpleFlowerPositions_;
    std::vector<Vec3> pinkFlowerPositions_;
};

} // namespace sky
