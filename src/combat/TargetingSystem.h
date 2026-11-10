#pragma once
#include "core/Math.h"
#include <vector>

namespace sky {

enum class LockState {
    NoTarget,
    Searching,
    Acquiring,
    Locked
};

class TargetingSystem {
public:
    TargetingSystem() = default;

    void reset();

    void cycleTarget(const std::vector<Vec3>& targetPositions, const std::vector<bool>& targetAlive);

    void update(double dt, const Vec3& playerPos, const Vec3& playerForward,
                const std::vector<Vec3>& targetPositions, const std::vector<bool>& targetAlive);

    LockState lockState() const { return lockState_; }
    int selectedTargetIndex() const { return selectedTargetIndex_; }
    void setSelectedTargetIndex(int index) { selectedTargetIndex_ = index; }

    double lockProgress() const {
        return acquireTime_ > 0.0 ? std::clamp(lockTimer_ / acquireTime_, 0.0, 1.0) : 0.0;
    }

    bool hasValidLock() const { return lockState_ == LockState::Locked; }

    double maxLockRange() const { return maxLockRange_; }
    double lockConeAngle() const { return lockConeAngle_; }
    double acquireTime() const { return acquireTime_; }

    void setMaxLockRange(double r) { maxLockRange_ = r; }
    void setLockConeAngle(double a) { lockConeAngle_ = a; }
    void setAcquireTime(double t) { acquireTime_ = t; }

private:
    int selectedTargetIndex_{-1};
    LockState lockState_{LockState::NoTarget};
    double lockTimer_{0.0};

    double maxLockRange_{1200.0};          // 1.2 km max targeting range
    double lockConeAngle_{radians(30.0)};  // 30 degree half-cone from nose
    double acquireTime_{0.80};             // 0.8s continuous focus to lock
};

} // namespace sky
