#include "TerrainChunk.h"
#include <algorithm>
#include <cmath>

namespace sky {

TerrainChunk::TerrainChunk(int chunkX, int chunkZ, double minX, double minZ)
    : chunkX_(chunkX), chunkZ_(chunkZ), minX_(minX), maxX_(minX + chunkSize),
      minZ_(minZ), maxZ_(minZ + chunkSize),
      center_{(minX_ + maxX_) * 0.5, 0.0, (minZ_ + maxZ_) * 0.5} {}

double TerrainChunk::distanceTo(const Vec3& p) const {
    double dx = std::max({minX_ - p.x, 0.0, p.x - maxX_});
    double dz = std::max({minZ_ - p.z, 0.0, p.z - maxZ_});
    return std::sqrt(dx * dx + dz * dz);
}

int TerrainChunk::updateLOD(double distance) {
    // Hysteresis thresholds:
    // LOD 0 -> 1 at 650m, 1 -> 0 at 550m
    // LOD 1 -> 2 at 1200m, 2 -> 1 at 1100m
    if (currentLOD_ == 0) {
        if (distance > 650.0) currentLOD_ = 1;
    } else if (currentLOD_ == 1) {
        if (distance < 550.0) currentLOD_ = 0;
        else if (distance > 1200.0) currentLOD_ = 2;
    } else { // currentLOD_ == 2
        if (distance < 1100.0) currentLOD_ = 1;
    }
    return currentLOD_;
}

void TerrainChunk::buildMeshes(const IHeightProvider& heightProvider, const BiomeSystem& biomes, uint32_t seed) {
    (void)seed;
    // LOD 0: 25m cells (20x20 cells = 21x21 verts = 800 tris)
    // LOD 1: 50m cells (10x10 cells = 11x11 verts = 200 tris)
    // LOD 2: 100m cells (5x5 cells = 6x6 verts = 50 tris)
    static constexpr int cellsPerAxis[numLODs] = {20, 10, 5};

    for (int lod = 0; lod < numLODs; ++lod) {
        int cells = cellsPerAxis[lod];
        int verts = cells + 1;
        double step = chunkSize / double(cells);

        auto& m = lodMeshes_[lod];
        m.vertices.clear();
        m.triangles.clear();
        m.indices.clear();
        m.vertices.reserve(verts * verts);
        m.triangles.reserve(cells * cells * 2);
        m.indices.reserve(cells * cells * 6);

        // Generate vertices
        for (int iz = 0; iz < verts; ++iz) {
            double z = minZ_ + iz * step;
            for (int ix = 0; ix < verts; ++ix) {
                double x = minX_ + ix * step;
                double y = heightProvider.groundHeight(x, z);

                TerrainVertex v;
                v.position = {x, y, z};
                v.normal = {0, 1, 0};
                v.color = 0;
                m.vertices.push_back(v);
            }
        }

        // Generate triangles
        for (int iz = 0; iz < cells; ++iz) {
            for (int ix = 0; ix < cells; ++ix) {
                uint32_t v00 = uint32_t(iz * verts + ix);
                uint32_t v10 = uint32_t(iz * verts + (ix + 1));
                uint32_t v01 = uint32_t((iz + 1) * verts + ix);
                uint32_t v11 = uint32_t((iz + 1) * verts + (ix + 1));

                // Triangle 0 (v00, v10, v11) covering fx >= fz
                {
                    Vec3 p0 = m.vertices[v00].position;
                    Vec3 p1 = m.vertices[v10].position;
                    Vec3 p2 = m.vertices[v11].position;
                    Vec3 n = normalized(cross(p1 - p0, p2 - p0));
                    Vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
                    uint32_t col = biomes.evaluatePackedColor(center.x, center.z, center.y, n);

                    m.triangles.push_back({v00, v10, v11, n, col});
                    m.indices.push_back(v00);
                    m.indices.push_back(v10);
                    m.indices.push_back(v11);
                }

                // Triangle 1 (v00, v11, v01) covering fx < fz
                {
                    Vec3 p0 = m.vertices[v00].position;
                    Vec3 p1 = m.vertices[v11].position;
                    Vec3 p2 = m.vertices[v01].position;
                    Vec3 n = normalized(cross(p1 - p0, p2 - p0));
                    Vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
                    uint32_t col = biomes.evaluatePackedColor(center.x, center.z, center.y, n);

                    m.triangles.push_back({v00, v11, v01, n, col});
                    m.indices.push_back(v00);
                    m.indices.push_back(v11);
                    m.indices.push_back(v01);
                }
            }
        }
    }
}

} // namespace sky
