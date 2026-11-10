#include "MouseSteering.h"

#include <algorithm>
#include <cmath>

namespace sky {

MouseSteering::Command MouseSteering::update(double nx, double ny, double dt) {
    rawX_ = std::clamp(nx, -1.0, 1.0);
    rawY_ = std::clamp(ny, -1.0, 1.0);

    // Radial deadzone: inside the radius the command is zero; past it the
    // magnitude is remapped continuously from 0..1 so there is no jump.
    double targetYaw = 0.0, targetPitch = 0.0;
    const double mag = std::hypot(rawX_, rawY_);
    if (mag > config_.deadzone && mag > 1e-9) {
        const double scaled = (std::min(mag, 1.0) - config_.deadzone) / (1.0 - config_.deadzone);
        const double curved = std::pow(scaled, config_.responseExponent);
        const double s = curved / mag;
        targetYaw = rawX_ * s * config_.mouseYawSensitivity;
        targetPitch = rawY_ * s * config_.mousePitchSensitivity;
    }
    if (config_.invertMouseY) targetPitch = -targetPitch;

    // Frame-rate independent exponential smoothing.
    const double alpha = dt > 0.0 ? 1.0 - std::exp(-config_.smoothingSharpness * dt) : 1.0;
    smoothYaw_ += (targetYaw - smoothYaw_) * alpha;
    smoothPitch_ += (targetPitch - smoothPitch_) * alpha;

    return {std::clamp(smoothPitch_, -1.0, 1.0), std::clamp(smoothYaw_, -1.0, 1.0)};
}

void MouseSteering::reset() {
    rawX_ = rawY_ = 0.0;
    smoothPitch_ = smoothYaw_ = 0.0;
}

} // namespace sky
