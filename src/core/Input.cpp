#include "Input.h"

#include <algorithm>

namespace sky {

InputState Input::update(const KeyboardState& k) {
    InputState result{
        {double(k.w) - double(k.s), double(k.d) - double(k.a),
         double(k.shift) - double(k.ctrl), k.shift && !k.ctrl},
        k.reset && !resetHeld_,
        k.escape,
        k.space,
        k.f && !fHeld_,
        k.tab && !tabHeld_
    };
    resetHeld_ = k.reset;
    fHeld_ = k.f;
    tabHeld_ = k.tab;
    return result;
}

InputState Input::update(const KeyboardState& k, const MouseState& m, double dt) {
    InputState result{};

    // Normalise the cursor against the client centre (prompt §3), then let
    // MouseSteering apply deadzone, response curve and smoothing. A window
    // that lost focus contributes nothing and resets the smoothing state,
    // so returning to the game never produces a stale-input jump.
    MouseSteering::Command cmd{};
    if (m.active && m.clientWidth > 0 && m.clientHeight > 0) {
        const double nx = std::clamp((double(m.x) - m.clientWidth * 0.5) / (m.clientWidth * 0.5), -1.0, 1.0);
        const double ny = std::clamp((m.clientHeight * 0.5 - double(m.y)) / (m.clientHeight * 0.5), -1.0, 1.0);
        cmd = steering_.update(nx, ny, dt);
    } else {
        steering_.reset();
    }
    result.mouseNX = steering_.rawX();
    result.mouseNY = steering_.rawY();

    const MouseFlightConfig& cfg = steering_.config();
    const double keyboardPitch = (double(k.w) - double(k.s)) * cfg.keyboardPitchWeight;
    result.flight.pitch = std::clamp(cmd.pitch + keyboardPitch, -1.0, 1.0);
    result.flight.turn = cmd.yaw;
    result.flight.roll = (double(k.a) - double(k.d)) * cfg.manualRollWeight;
    result.flight.throttle = double(k.shift) - double(k.ctrl);
    result.flight.boost = k.shift && !k.ctrl;

    result.fireGun = k.space || (m.active && m.left);
    result.lockOn = m.active && m.right;
    result.fireMissile = k.f && !fHeld_;
    result.switchTarget = k.tab && !tabHeld_;
    result.reset = k.reset && !resetHeld_;
    result.pausePressed = k.escape && !escHeld_;
    result.switchCamera = k.c && !cHeld_;
    result.quit = false; // Esc pauses in this scheme; closing the window quits

    fHeld_ = k.f;
    tabHeld_ = k.tab;
    resetHeld_ = k.reset;
    escHeld_ = k.escape;
    cHeld_ = k.c;
    return result;
}

} // namespace sky
