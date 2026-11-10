#include "ChaseCamera.h"
#include "aircraft/FlightController.h"
#include "world/HeightProvider.h"
#include <algorithm>

namespace sky {

CameraPose ChaseCamera::desired(const Aircraft& a) const {
    double speed = std::clamp((a.speed - FlightTuning::minSpeed) / (FlightTuning::maxSpeed - FlightTuning::minSpeed), 0.0, 1.0);
    // Aircraft up keeps camera well-defined through vertical flight and full loops.
    return {a.transform.position - a.transform.forward() * (24 + 6 * speed) + a.transform.up() * 8,
            a.transform.position + a.transform.forward() * (18 + 20 * speed),
            a.transform.up(),
            65 + 15 * speed};
}

void ChaseCamera::applyTerrainAvoidance(const IHeightProvider* terrain) {
    if (!terrain) return;
    double groundY = terrain->groundHeight(pose_.position.x, pose_.position.z);
    double minCamY = groundY + WorldConfig::cameraGroundMargin;
    if (pose_.position.y < minCamY) {
        pose_.position.y = minCamY;
        Vec3 view = normalized(pose_.target - pose_.position);
        Vec3 up = pose_.up - view * dot(pose_.up, view);
        if (length(up) > 1e-6) {
            pose_.up = normalized(up);
        }
    }
}

void ChaseCamera::reset(const Aircraft& a, const IHeightProvider* terrain) {
    pose_ = desired(a);
    applyTerrainAvoidance(terrain);
}

void ChaseCamera::update(const Aircraft& a, double dt, const IHeightProvider* terrain) {
    auto target = desired(a);
    pose_.position = lerp(pose_.position, target.position, response(6, dt));
    pose_.target = lerp(pose_.target, target.target, response(8, dt));

    // Project up off the view axis; fallback protects singular transient views.
    Vec3 view = normalized(pose_.target - pose_.position);
    Vec3 up = lerp(pose_.up, target.up, response(8, dt));
    up = up - view * dot(up, view);
    if (length(up) < 1e-6) {
        up = cross(view, {1, 0, 0});
        if (length(up) < 1e-6) up = cross(view, {0, 1, 0});
    }
    pose_.up = normalized(up);
    pose_.fovDegrees += (target.fovDegrees - pose_.fovDegrees) * response(4, dt);

    applyTerrainAvoidance(terrain);
}

} // namespace sky
