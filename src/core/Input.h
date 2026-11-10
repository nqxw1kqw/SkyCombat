#pragma once
#include "MouseSteering.h"

namespace sky {

struct KeyboardState {
    bool w{}, s{}, a{}, d{}, shift{}, ctrl{}, reset{}, escape{};
    bool space{}, f{}, tab{};
    bool c{};
};

// Mouse snapshot for one render frame, taken by the app layer. The app
// captures the cursor and re-centres it every frame, so deltaX/deltaY are
// this frame's relative movement in pixels (Y down). active is true only
// while gameplay capture is on (focused and not paused).
struct MouseState {
    int deltaX{}, deltaY{};
    bool left{}, right{};
    bool active{};
};

struct FlightInput {
    double pitch{}, turn{}, throttle{};
    bool boost{};
    double roll{}; // manual roll command (A/D), + = roll left
    // Direct mouse rotation for this step, in radians (Direct Mouse
    // Flight). Zero for keyboard-only and AI inputs.
    double mouseYaw{};
    double mousePitch{};
};

struct InputState {
    FlightInput flight;
    bool reset{}, quit{};
    bool fireGun{};
    bool fireMissile{};
    bool switchTarget{};
    // Mouse + Keyboard control additions
    bool lockOn{};        // right mouse held: targeting may acquire
    bool pausePressed{};  // Esc edge
    bool switchCamera{};  // C edge (cycles camera presets)
};

class Input {
public:
    // Legacy keyboard-only mapping (kept for the portable tests and as a
    // fallback): W/S pitch, A/D turn, Shift/Ctrl throttle, Esc quits.
    InputState update(const KeyboardState& keys);
    // Mouse + Keyboard mapping (Direct Mouse Flight): the mouse rotates
    // the aircraft directly through MouseSteering, W/S add pitch, A/D roll
    // manually, left mouse fires, right mouse holds lock-on, Esc pauses
    // (edge), C cycles the chase-camera preset.
    InputState update(const KeyboardState& keys, const MouseState& mouse, double dt);

private:
    MouseSteering steering_;
    bool resetHeld_{};
    bool fHeld_{};
    bool tabHeld_{};
    bool escHeld_{};
    bool cHeld_{};
};

} // namespace sky
