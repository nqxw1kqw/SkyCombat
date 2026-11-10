#pragma once
namespace sky {
struct KeyboardState { bool w{},s{},a{},d{},shift{},ctrl{},reset{},escape{}; };
struct FlightInput { double pitch{},turn{},throttle{}; bool boost{}; };
struct InputState { FlightInput flight; bool reset{},quit{}; };
class Input {
public:
    InputState update(const KeyboardState& keys);
private:
    bool resetHeld_{};
};
}
