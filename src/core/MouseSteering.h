#pragma once

namespace sky {

// Tuning for Direct Mouse Flight (relative mouse control): the mouse
// reports per-frame movement in pixels, converted here to rotation in
// radians. Rates and limits of the airframe itself live in FlightTuning.
struct MouseFlightConfig {
    double mouseSensitivityX = 0.0022; // radians of yaw per pixel
    double mouseSensitivityY = 0.0022; // radians of pitch per pixel
    bool invertY = false;              // flip mouse pitch direction
    double smoothingSharpness = 12.0;  // 1/s, release rate of pending rotation
    double maxDeltaPerFrame = 0.35;    // radians; spike clamp per render frame
    double keyboardPitchWeight = 0.75; // W/S contribution next to the mouse
    double manualRollWeight = 1.0;     // A/D manual roll strength
};

// Relative mouse steering: each frame's pixel delta becomes a rotation
// command (radians). Incoming rotation is clamped per frame (focus or
// recenter spikes can never teleport the nose), accumulated, and released
// through a frame-rate independent exponential filter that conserves the
// total angle — smoothing spreads a flick over a few frames without
// eating any of it. A still mouse produces exactly zero command, so the
// aircraft simply holds its heading.
class MouseSteering {
public:
    struct Command {
        double mouseYaw{};   // radians to yaw this step (+ = same sense as turn +)
        double mousePitch{}; // radians to pitch this step (+ = nose up)
    };

    explicit MouseSteering(const MouseFlightConfig& config = {}) : config_(config) {}

    // deltaX/deltaY are raw screen pixels (Y down, as the OS reports).
    Command update(double deltaX, double deltaY, double dt);
    void reset();

    const MouseFlightConfig& config() const { return config_; }

private:
    MouseFlightConfig config_;
    double pendingYaw_{};
    double pendingPitch_{};
};

} // namespace sky
