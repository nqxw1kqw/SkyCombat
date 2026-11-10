#include "TerrainRenderer.h"
#include <DxLib.h>

namespace sky {

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

} // namespace sky
