#pragma once
#include "core/Math.h"
#include <cstdint>

namespace sky {
struct WorldConfig {
    static constexpr double worldSize = 2000.0;
    static constexpr double halfSize = worldSize / 2.0;
    static constexpr double minX = -halfSize;
    static constexpr double maxX =  halfSize;
    static constexpr double minZ = -halfSize;
    static constexpr double maxZ =  halfSize;

    static constexpr double cellSize = 25.0;
    static constexpr int numCellsX = int(worldSize / cellSize); // 80 cells
    static constexpr int numCellsZ = int(worldSize / cellSize); // 80 cells
    static constexpr int numVertsX = numCellsX + 1;             // 81 vertices
    static constexpr int numVertsZ = numCellsZ + 1;             // 81 vertices

    static constexpr uint32_t fixedSeed = 1337;

    static constexpr double safeClearance = 8.0;
    static constexpr double warningClearance = 25.0;
    static constexpr double cameraGroundMargin = 2.5;

    static constexpr Vec3 spawnPosition{0.0, 120.0, 0.0};
};
}
