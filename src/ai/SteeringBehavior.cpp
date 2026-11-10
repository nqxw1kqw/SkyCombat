#include "SteeringBehavior.h"
#include "world/HeightProvider.h"
#include "aircraft/FlightController.h"
#include <algorithm>
#include <cmath>

namespace sky {

namespace {

FlightInput steerTowards(const Aircraft& aircraft, const Vec3& targetPos, double desiredSpeed,
                         const IHeightProvider* terrain) {
    Vec3 pos = aircraft.transform.position;
    Vec3 effectiveTarget = targetPos;

    // 1. World boundary soft turn: steer towards world center if near border
    if (std::abs(pos.x) > 850.0 || std::abs(pos.z) > 850.0) {
        effectiveTarget = Vec3{0.0, 120.0, 0.0};
    }

    Vec3 toTarget = effectiveTarget - pos;
    Vec3 forward = aircraft.transform.forward();
    Vec3 right = aircraft.transform.right();
    Vec3 up = aircraft.transform.up();

    double xLocal = dot(toTarget, right);
    double yLocal = dot(toTarget, up);
    double zLocal = dot(toTarget, forward);

    FlightInput input{};

    // Turn steering (A/D)
    if (zLocal < 0.0) {
        input.turn = (xLocal >= 0.0) ? 1.0 : -1.0;
    } else {
        input.turn = std::clamp(xLocal / (zLocal + 10.0) * 2.5, -1.0, 1.0);
    }

    // Pitch steering (W/S)
    input.pitch = std::clamp(yLocal / (std::abs(zLocal) + 15.0) * 2.5, -1.0, 1.0);

    // Throttle steering
    double speedDiff = desiredSpeed - aircraft.speed;
    if (speedDiff > 5.0) {
        input.throttle = 1.0;
        if (speedDiff > 25.0) input.boost = true;
    } else if (speedDiff < -5.0) {
        input.throttle = -1.0;
    } else {
        input.throttle = 0.0;
    }

    // 2. Terrain avoidance override
    if (terrain) {
        double groundY = terrain->groundHeight(pos.x, pos.z);
        double clearance = pos.y - groundY;
        if (clearance < 35.0) {
            // Force pitch UP and throttle up when low
            input.pitch = 1.0;
            input.throttle = 1.0;
            input.boost = true;
        }
    }

    return input;
}

} // namespace

FlightInput SteeringBehavior::seek(const Aircraft& aircraft, const Vec3& targetPos, double desiredSpeed,
                                   const IHeightProvider* terrain) {
    return steerTowards(aircraft, targetPos, desiredSpeed, terrain);
}

FlightInput SteeringBehavior::flee(const Aircraft& aircraft, const Vec3& threatPos, double desiredSpeed,
                                   const IHeightProvider* terrain) {
    Vec3 awayDir = normalized(aircraft.transform.position - threatPos);
    Vec3 fleeTarget = aircraft.transform.position + awayDir * 500.0 + Vec3{0, 40, 0};
    return steerTowards(aircraft, fleeTarget, desiredSpeed, terrain);
}

FlightInput SteeringBehavior::evade(const Aircraft& aircraft, const Vec3& threatPos, const Vec3& threatForward,
                                    double desiredSpeed, const IHeightProvider* terrain) {
    // Break perpendicular to threat heading with altitude gain
    Vec3 perp = normalized(cross(threatForward, Vec3{0, 1, 0}));
    if (length(perp) < 1e-4) perp = Vec3{1, 0, 0};

    // Pick side further away or alternate
    double sign = dot(aircraft.transform.position - threatPos, perp) >= 0.0 ? 1.0 : -1.0;
    Vec3 evadeTarget = aircraft.transform.position + perp * (sign * 400.0) + Vec3{0, 50, 0};
    FlightInput input = steerTowards(aircraft, evadeTarget, desiredSpeed, terrain);
    input.boost = true;
    return input;
}

} // namespace sky
