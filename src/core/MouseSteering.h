#pragma once

namespace sky {

// Tuning for mouse flight steering (Mouse + Keyboard control prompt, §5).
// Angles are unitless commands in [-1, 1]; rates live in FlightController.
struct MouseFlightConfig {
    double deadzone = 0.08;            // radial deadzone, normalised units;
    double responseExponent = 1.4;     // response curve power
    double smoothingSharpness = 12.0;  // 1/s, frame-rate independent smoothing
    double mouseYawSensitivity = 1.0;
    double mousePitchSensitivity = 1.0;
    double keyboardPitchWeight = 0.75; // W/S contribution next to the mouse
    double manualRollWeight = 1.0;     // A/D manual roll strength
    bool invertMouseY = false;
};

// Converts a normalised cursor offset (x right, y up, each in [-1, 1],
// measured from the window centre) into pitch/yaw commands: radial
// deadzone with continuous remap, a power response curve, and exponential
// smoothing that behaves the same at any frame rate.
class MouseSteering {
public:
    struct Command {
        double pitch{};
        double yaw{};
    };

    explicit MouseSteering(const MouseFlightConfig& config = {}) : config_(config) {}

    Command update(double nx, double ny, double dt);
    void reset();

    const MouseFlightConfig& config() const { return config_; }
    double rawX() const { return rawX_; } // last normalised input, for the reticle
    double rawY() const { return rawY_; }

private:
    MouseFlightConfig config_;
    double rawX_{};
    double rawY_{};
    double smoothPitch_{};
    double smoothYaw_{};
};

} // namespace sky
