#pragma once

namespace sky {

struct KeyboardState {
    bool w{}, s{}, a{}, d{}, shift{}, ctrl{}, reset{}, escape{};
    bool space{}, f{}, tab{};
};

struct FlightInput {
    double pitch{}, turn{}, throttle{};
    bool boost{};
};

struct InputState {
    FlightInput flight;
    bool reset{}, quit{};
    bool fireGun{};
    bool fireMissile{};
    bool switchTarget{};
};

class Input {
public:
    InputState update(const KeyboardState& keys);
private:
    bool resetHeld_{};
    bool fHeld_{};
    bool tabHeld_{};
};

} // namespace sky
