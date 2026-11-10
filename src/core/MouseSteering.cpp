#include "MouseSteering.h"

#include <algorithm>
#include <cmath>

namespace sky {

MouseSteering::Command MouseSteering::update(double deltaX, double deltaY, double dt) {
    // Pixels to radians. Screen Y points down, so moving the mouse up is a
    // negative deltaY and must raise the nose: pitch = -deltaY * sensY.
    double rawYaw = std::clamp(deltaX * config_.mouseSensitivityX,
                               -config_.maxDeltaPerFrame, config_.maxDeltaPerFrame);
    double rawPitch = std::clamp(-deltaY * config_.mouseSensitivityY,
                                 -config_.maxDeltaPerFrame, config_.maxDeltaPerFrame);
    if (config_.invertY) rawPitch = -rawPitch;

    pendingYaw_ += rawYaw;
    pendingPitch_ += rawPitch;

    // Release a frame-rate independent fraction of the pending rotation.
    // Whatever is not released stays pending, so the total applied angle
    // always equals the total mouse angle (minus nothing).
    const double alpha = dt > 0.0
        ? (config_.smoothingSharpness > 0.0
               ? 1.0 - std::exp(-config_.smoothingSharpness * dt)
               : 1.0)
        : 1.0;
    Command cmd{pendingYaw_ * alpha, pendingPitch_ * alpha};
    pendingYaw_ -= cmd.mouseYaw;
    pendingPitch_ -= cmd.mousePitch;
    return cmd;
}

void MouseSteering::reset() {
    pendingYaw_ = 0.0;
    pendingPitch_ = 0.0;
}

} // namespace sky
