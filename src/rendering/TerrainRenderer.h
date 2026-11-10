#pragma once
#include "world/TerrainMesh.h"
#include "world/TerrainChunk.h"
#include "world/SkySystem.h"
#include <vector>

namespace sky {

class VegetationSystem;
class CloudSystem;

class TerrainRenderer {
public:
    // Configures DXLib distance fog and background state
    void setupEnvironment(const SkySystem& sky) const;

    // Draws raw terrain mesh data
    void draw(const TerrainMeshData& mesh) const;

    // Draws active LOD meshes from the 16 terrain chunks
    void drawChunks(const std::vector<TerrainChunk>& chunks) const;

    // Draws distant northern mountain silhouettes
    void drawDistantMountains(const TerrainMeshData& mountains) const;

    // Draws batched vegetation geometry
    void drawVegetation(const VegetationSystem& vegetation) const;

    // Draws low-poly cloud clusters mesh
    void drawClouds(const CloudSystem& clouds) const;
};

} // namespace sky
