#pragma once
#include "aircraft/Aircraft.h"
#include "core/Input.h"

namespace sky {

class IHeightProvider;

class SteeringBehavior {
public:
    static FlightInput seek(const Aircraft& aircraft, const Vec3& targetPos, double desiredSpeed,
                            const IHeightProvider* terrain = nullptr);

    static FlightInput flee(const Aircraft& aircraft, const Vec3& threatPos, double desiredSpeed,
                            const IHeightProvider* terrain = nullptr);

    static FlightInput evade(const Aircraft& aircraft, const Vec3& threatPos, const Vec3& threatForward,
                             double desiredSpeed, const IHeightProvider* terrain = nullptr);
};

} // namespace sky
