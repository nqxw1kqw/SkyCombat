#pragma once
#include "world/WorldConfig.h"

namespace sky {

class SpeedEffects {
public:
    explicit SpeedEffects(bool enabled = true,
                          double threshold = WorldConfig::speedLineThreshold,
                          double maxSpeed = 140.0)
        : enabled_(enabled), threshold_(threshold), maxSpeed_(maxSpeed) {}

    // Pure intensity evaluator: returns 0.0 below threshold, ramps to 1.0 above
    double evaluateIntensity(double speed, bool boost = false) const;

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

    double threshold() const { return threshold_; }
    void setThreshold(double threshold) { threshold_ = threshold; }

    double maxSpeed() const { return maxSpeed_; }

    // DXLib layer: draws radial speed lines radiating outward from screen center
    void draw(double intensity, int screenWidth = 1280, int screenHeight = 720) const;

private:
    bool enabled_{true};
    double threshold_{WorldConfig::speedLineThreshold};
    double maxSpeed_{140.0};
};

} // namespace sky
