#include "TargetingSystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

void TargetingSystem::reset() {
    selectedTargetIndex_ = -1;
    lockState_ = LockState::NoTarget;
    lockTimer_ = 0.0;
}

void TargetingSystem::cycleTarget(const std::vector<Vec3>& targetPositions, const std::vector<bool>& targetAlive) {
    if (targetPositions.empty() || targetPositions.size() != targetAlive.size()) {
        selectedTargetIndex_ = -1;
        lockState_ = LockState::NoTarget;
        lockTimer_ = 0.0;
        return;
    }

    size_t count = targetPositions.size();
    size_t start = (selectedTargetIndex_ >= 0) ? size_t(selectedTargetIndex_ + 1) : 0;

    for (size_t offset = 0; offset < count; ++offset) {
        size_t idx = (start + offset) % count;
        if (targetAlive[idx]) {
            if (selectedTargetIndex_ != int(idx)) {
                selectedTargetIndex_ = int(idx);
                lockTimer_ = 0.0;
                lockState_ = LockState::Searching;
            }
            return;
        }
    }

    // No living target found
    selectedTargetIndex_ = -1;
    lockState_ = LockState::NoTarget;
    lockTimer_ = 0.0;
}

void TargetingSystem::update(double dt, const Vec3& playerPos, const Vec3& playerForward,
                            const std::vector<Vec3>& targetPositions, const std::vector<bool>& targetAlive) {
    if (targetPositions.empty() || targetPositions.size() != targetAlive.size()) {
        selectedTargetIndex_ = -1;
        lockState_ = LockState::NoTarget;
        lockTimer_ = 0.0;
        return;
    }

    // Validate current target
    if (selectedTargetIndex_ < 0 || selectedTargetIndex_ >= int(targetPositions.size()) ||
        !targetAlive[selectedTargetIndex_]) {
        // Find first living target if available
        selectedTargetIndex_ = -1;
        for (size_t i = 0; i < targetAlive.size(); ++i) {
            if (targetAlive[i]) {
                selectedTargetIndex_ = int(i);
                break;
            }
        }
        lockTimer_ = 0.0;
        if (selectedTargetIndex_ < 0) {
            lockState_ = LockState::NoTarget;
            return;
        }
    }

    const Vec3& targetPos = targetPositions[selectedTargetIndex_];
    Vec3 toTarget = targetPos - playerPos;
    double dist = length(toTarget);

    if (dist > maxLockRange_ || dist < 1e-4) {
        lockTimer_ = 0.0;
        lockState_ = LockState::Searching;
        return;
    }

    Vec3 dirToTarget = normalized(toTarget);
    double dotVal = std::clamp(dot(playerForward, dirToTarget), -1.0, 1.0);
    double angle = std::acos(dotVal);

    if (angle <= lockConeAngle_) {
        lockTimer_ += dt;
        if (lockTimer_ >= acquireTime_) {
            lockState_ = LockState::Locked;
        } else {
            lockState_ = LockState::Acquiring;
        }
    } else {
        lockTimer_ = 0.0;
        lockState_ = LockState::Searching;
    }
}

} // namespace sky
