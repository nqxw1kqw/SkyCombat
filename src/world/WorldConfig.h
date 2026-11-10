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

    // M4 Environment & Polish parameters
    static constexpr int maxGrassClumps = 1200;
    static constexpr int maxPurpleFlowerTufts = 400;
    static constexpr int maxPinkFlowerTufts = 400;

    static constexpr int numCloudClusters = 18;
    static constexpr double cloudMinAltitude = 230.0;
    static constexpr double cloudMaxAltitude = 360.0;
    static constexpr double cloudDriftSpeed = 2.5; // m/s

    static constexpr double speedLineThreshold = 105.0; // m/s
    static constexpr double cameraRollFraction = 0.20;
    static constexpr double cameraMaxRoll = radians(12.0);
};
}
