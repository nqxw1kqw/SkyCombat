#pragma once
#include "TerrainMesh.h"
#include "HeightProvider.h"
#include "BiomeSystem.h"
#include <array>

namespace sky {

class TerrainChunk {
public:
    static constexpr int numLODs = 3;
    static constexpr double chunkSize = 500.0;

    TerrainChunk(int chunkX, int chunkZ, double minX, double minZ);

    int chunkX() const { return chunkX_; }
    int chunkZ() const { return chunkZ_; }
    double minX() const { return minX_; }
    double maxX() const { return maxX_; }
    double minZ() const { return minZ_; }
    double maxZ() const { return maxZ_; }
    Vec3 center() const { return center_; }

    // Generates the 3 LOD meshes using the height provider and biome system
    void buildMeshes(const IHeightProvider& heightProvider, const BiomeSystem& biomes, uint32_t seed);

    // Distance from a point (e.g. camera) to the chunk's horizontal bounding box
    double distanceTo(const Vec3& point) const;

    // Updates LOD level using distance and hysteresis thresholds
    int updateLOD(double distance);

    int currentLOD() const { return currentLOD_; }
    void forceLOD(int lod) { currentLOD_ = std::clamp(lod, 0, numLODs - 1); }

    const TerrainMeshData& mesh(int lod) const { return lodMeshes_[std::clamp(lod, 0, numLODs - 1)]; }
    const TerrainMeshData& activeMesh() const { return lodMeshes_[currentLOD_]; }

private:
    int chunkX_{0};
    int chunkZ_{0};
    double minX_{0};
    double maxX_{0};
    double minZ_{0};
    double maxZ_{0};
    Vec3 center_{0, 0, 0};
    int currentLOD_{0};

    std::array<TerrainMeshData, numLODs> lodMeshes_;
};

} // namespace sky
