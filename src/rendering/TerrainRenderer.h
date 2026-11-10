#pragma once
#include "world/TerrainSystem.h"

namespace sky {

class TerrainRenderer {
public:
    void draw(const TerrainMeshData& mesh) const;
};

}
