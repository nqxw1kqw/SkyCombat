#pragma once
#include "core/Math.h"
#include "CombatEvent.h"
#include <vector>

namespace sky {

class IHeightProvider;

class Missile {
public:
    Missile() = default;

    void launch(const Vec3& startPos, const Vec3& startForward, int targetId, double initialSpeed = 160.0);
    bool update(double dt, const Vec3& targetPos, bool targetValid, const IHeightProvider* terrain,
                std::vector<CombatEvent>& outEvents);

    bool isActive() const { return active_; }
    void deactivate() { active_ = false; }

    const Vec3& position() const { return position_; }
    const Vec3& forward() const { return forward_; }
    int targetId() const { return targetId_; }
    double damage() const { return damage_; }
    double speed() const { return speed_; }
    double turnRateLimit() const { return turnRateLimit_; }
    double lifetime() const { return lifetime_; }
    double maxLifetime() const { return maxLifetime_; }
    double proximityRadius() const { return proximityRadius_; }

    const std::vector<Vec3>& trail() const { return trail_; }

private:
    Vec3 position_{};
    Vec3 forward_{0, 0, 1};
    int targetId_{-1};
    bool active_{false};

    double speed_{160.0};
    double turnRateLimit_{radians(45.0)}; // rad/s (45 deg/s max turn rate)
    double lifetime_{0.0};
    double maxLifetime_{7.0};             // seconds
    double proximityRadius_{6.0};         // meters
    double damage_{80.0};

    std::vector<Vec3> trail_;
    double trailTimer_{0.0};
};

} // namespace sky
