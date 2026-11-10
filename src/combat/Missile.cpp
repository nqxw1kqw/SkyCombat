#include "Missile.h"
#include "world/HeightProvider.h"
#include <algorithm>
#include <cmath>

namespace sky {

void Missile::launch(const Vec3& startPos, const Vec3& startForward, int targetId, double initialSpeed) {
    position_ = startPos;
    forward_ = normalized(startForward);
    targetId_ = targetId;
    speed_ = std::max(initialSpeed, 160.0);
    active_ = true;
    lifetime_ = 0.0;
    trailTimer_ = 0.0;
    trail_.clear();
    trail_.push_back(position_);
}

bool Missile::update(double dt, const Vec3& targetPos, bool targetValid, const IHeightProvider* terrain,
                     std::vector<CombatEvent>& outEvents) {
    if (!active_ || dt <= 0.0) return active_;

    lifetime_ += dt;
    if (lifetime_ >= maxLifetime_) {
        active_ = false;
        outEvents.push_back({CombatEventType::MissileDetonated, position_, targetId_, 0.0});
        return false;
    }

    // Substep integration for anti-tunneling
    constexpr int numSubsteps = 4;
    double subDt = dt / double(numSubsteps);

    for (int step = 0; step < numSubsteps; ++step) {
        // Homing steering with hard turn rate limit
        if (targetValid) {
            Vec3 toTarget = targetPos - position_;
            double dist = length(toTarget);
            if (dist > 1e-4) {
                Vec3 desiredDir = normalized(toTarget);
                double dotVal = std::clamp(dot(forward_, desiredDir), -1.0, 1.0);
                double angle = std::acos(dotVal);
                double maxTurn = turnRateLimit_ * subDt;

                if (angle <= maxTurn) {
                    forward_ = desiredDir;
                } else {
                    Vec3 axis = cross(forward_, desiredDir);
                    if (length(axis) < 1e-6) {
                        axis = cross(forward_, Vec3{0, 1, 0});
                        if (length(axis) < 1e-6) axis = cross(forward_, Vec3{1, 0, 0});
                    }
                    forward_ = Quat::axisAngle(normalized(axis), maxTurn).rotate(forward_);
                    forward_ = normalized(forward_);
                }
            }
        }

        // Advance position
        position_ = position_ + forward_ * (speed_ * subDt);

        // Proximity detonation check
        if (targetValid && length(position_ - targetPos) <= proximityRadius_) {
            active_ = false;
            outEvents.push_back({CombatEventType::MissileDetonated, position_, targetId_, damage_});
            return false;
        }

        // Terrain collision check
        if (terrain && position_.y <= terrain->groundHeight(position_.x, position_.z)) {
            active_ = false;
            outEvents.push_back({CombatEventType::MissileDetonated, position_, targetId_, 0.0});
            return false;
        }
    }

    // Record trail history
    trailTimer_ += dt;
    if (trailTimer_ >= 0.04) {
        trailTimer_ = 0.0;
        trail_.push_back(position_);
        if (trail_.size() > 32) {
            trail_.erase(trail_.begin());
        }
    }

    return true;
}

} // namespace sky
