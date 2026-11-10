#pragma once
#include "Aircraft.h"
#include "core/Input.h"

namespace sky {

class IHeightProvider;

struct FlightTuning {
    static constexpr double minSpeed = 25, cruiseSpeed = 70, maxSpeed = 140;
    static constexpr double pitchRate = radians(65), yawRate = radians(55), maxBank = radians(50);
    static constexpr double levelResponse = 3.5, acceleration = 25, deceleration = 30;
    static constexpr double safeClearance = 8.0;
    static constexpr double warningClearance = 25.0;
    static constexpr double maxAssistPitchRate = radians(45);
};

class FlightController {
public:
    void update(Aircraft& aircraft, const FlightInput& input, double dt, const IHeightProvider* terrain = nullptr) const;
};

}
