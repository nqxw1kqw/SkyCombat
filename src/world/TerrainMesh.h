#pragma once
#include "core/Math.h"
#include <vector>
#include <cstdint>

namespace sky {

struct TerrainVertex {
    Vec3 position;
    Vec3 normal{0, 1, 0};
    uint32_t color{0};
};

struct TerrainTriangle {
    uint32_t i0{}, i1{}, i2{};
    Vec3 normal{0, 1, 0};
    uint32_t color{0};
};

struct TerrainMeshData {
    std::vector<TerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<TerrainTriangle> triangles;
};

} // namespace sky
