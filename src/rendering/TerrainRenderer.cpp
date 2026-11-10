#include "TerrainRenderer.h"
#include "world/VegetationSystem.h"
#include "world/CloudSystem.h"
#include <DxLib.h>
#include <algorithm>

namespace sky {

void TerrainRenderer::setupEnvironment(const SkySystem& sky) const {
    const auto& fog = sky.fog();
    SetFogEnable(fog.enabled ? TRUE : FALSE);
    if (fog.enabled) {
        int r = std::clamp(int(fog.color.x * 255.0 + 0.5), 0, 255);
        int g = std::clamp(int(fog.color.y * 255.0 + 0.5), 0, 255);
        int b = std::clamp(int(fog.color.z * 255.0 + 0.5), 0, 255);
        SetFogColor(r, g, b);
        SetFogStartEnd(float(fog.nearDistance), float(fog.farDistance));
    }
}

void TerrainRenderer::draw(const TerrainMeshData& mesh) const {
    for (const auto& tri : mesh.triangles) {
        const auto& p0 = mesh.vertices[tri.i0].position;
        const auto& p1 = mesh.vertices[tri.i1].position;
        const auto& p2 = mesh.vertices[tri.i2].position;

        VECTOR v0 = VGet(float(p0.x), float(p0.y), float(p0.z));
        VECTOR v1 = VGet(float(p1.x), float(p1.y), float(p1.z));
        VECTOR v2 = VGet(float(p2.x), float(p2.y), float(p2.z));

        DrawTriangle3D(v0, v1, v2, tri.color, TRUE);
    }
}

void TerrainRenderer::drawChunks(const std::vector<TerrainChunk>& chunks) const {
    for (const auto& chunk : chunks) {
        draw(chunk.activeMesh());
    }
}

void TerrainRenderer::drawDistantMountains(const TerrainMeshData& mountains) const {
    draw(mountains);
}

void TerrainRenderer::drawVegetation(const VegetationSystem& vegetation) const {
    draw(vegetation.grassBatch());
    draw(vegetation.purpleFlowersBatch());
    draw(vegetation.pinkFlowersBatch());
}

void TerrainRenderer::drawClouds(const CloudSystem& clouds) const {
    draw(clouds.mesh());
}

} // namespace sky
