#include "WorldCollision.h"
#include <algorithm>

namespace sky {

double WorldCollision::clearance(const Aircraft& aircraft, const IHeightProvider& terrain) {
    double ground = terrain.groundHeight(aircraft.transform.position.x, aircraft.transform.position.z);
    return aircraft.transform.position.y - ground;
}

void WorldCollision::resolveAircraft(Aircraft& a, const IHeightProvider& terrain, double /*dt*/) {
    // Hard boundary containment
    a.transform.position.x = std::clamp(a.transform.position.x, WorldConfig::minX, WorldConfig::maxX);
    a.transform.position.z = std::clamp(a.transform.position.z, WorldConfig::minZ, WorldConfig::maxZ);

    double groundY = terrain.groundHeight(a.transform.position.x, a.transform.position.z);
    double clear = a.transform.position.y - groundY;
    a.clearance = clear;

    if (a.transform.position.y <= groundY) {
        // Ground collision: crash state, stop speed, clamp at or above mesh
        a.crashed = true;
        a.speed = 0.0;
        a.transform.position.y = groundY;
        a.clearance = 0.0;
        a.terrainWarning = true;
        a.terrainAssist = false;
    } else {
        a.terrainWarning = (clear < WorldConfig::safeClearance);
    }
}

void WorldCollision::clampCamera(CameraPose& pose, const IHeightProvider& terrain, double margin) {
    double ground = terrain.groundHeight(pose.position.x, pose.position.z);
    double minCamY = ground + margin;
    if (pose.position.y < minCamY) {
        pose.position.y = minCamY;
        // Re-orthogonalize up vector against view vector
        Vec3 view = normalized(pose.target - pose.position);
        Vec3 up = pose.up - view * dot(pose.up, view);
        if (length(up) > 1e-6) {
            pose.up = normalized(up);
        }
    }
}

} // namespace sky
