#pragma once
#include "HeightProvider.h"
#include "aircraft/Aircraft.h"
#include "camera/ChaseCamera.h"

namespace sky {

class WorldCollision {
public:
    // Calculates vertical clearance: aircraft Y - groundHeight(x, z)
    static double clearance(const Aircraft& aircraft, const IHeightProvider& terrain);

    // Resolves collision, clearance warning/assist states, and ground impact
    static void resolveAircraft(Aircraft& aircraft, const IHeightProvider& terrain, double dt);

    // Clamps camera position so it never sits below groundHeight + margin
    static void clampCamera(CameraPose& pose, const IHeightProvider& terrain, double margin = WorldConfig::cameraGroundMargin);
};

}
