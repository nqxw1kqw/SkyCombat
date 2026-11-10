#pragma once
#include "MouseSteering.h"

namespace sky {

struct KeyboardState {
    bool w{}, s{}, a{}, d{}, shift{}, ctrl{}, reset{}, escape{};
    bool space{}, f{}, tab{};
    bool c{};
};

// Raw mouse snapshot in window client pixels, taken by the app layer.
struct MouseState {
    int x{}, y{};
    int clientWidth{1280}, clientHeight{720};
    bool left{}, right{};
    bool active{}; // window focused; stale input is dropped when false
};

struct FlightInput {
    double pitch{}, turn{}, throttle{};
    bool boost{};
    double roll{}; // manual roll command (A/D), + = roll left
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
    bool switchCamera{};  // C edge
    double mouseNX{};     // raw normalised cursor offset, for the reticle
    double mouseNY{};
};

class Input {
public:
    // Legacy keyboard-only mapping (kept for the portable tests and as a
    // fallback): W/S pitch, A/D turn, Shift/Ctrl throttle, Esc quits.
    InputState update(const KeyboardState& keys);
    // Mouse + Keyboard mapping: the mouse steers pitch/yaw through
    // MouseSteering, W/S add pitch, A/D roll manually, left mouse fires,
    // right mouse holds lock-on, Esc pauses (edge), C switches camera.
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
