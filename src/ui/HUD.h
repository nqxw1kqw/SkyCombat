#pragma once
#include "aircraft/Aircraft.h"
#include <string>
#include <string_view>

namespace sky {

enum class FlightAssistState {
    Normal,
    AssistEngaged,
    LowClearanceWarning,
    Crashed
};

struct HUDData {
    double speedKmh{0.0};
    std::string speedString;
    double altitudeM{0.0};
    std::string altitudeString;
    double clearanceM{0.0};
    std::string clearanceString;
    FlightAssistState assistState{FlightAssistState::Normal};
    std::string assistStateString;
    double fps{0.0};
    std::string fpsString;
    std::string biomeString;
};

class HUD {
public:
    static double toKmh(double speedMs);
    static FlightAssistState evaluateAssistState(const Aircraft& a);
    static HUDData createData(const Aircraft& a, double fps, std::string_view biomeName);

    // DXLib rendering layer: only draws precomputed strings from HUDData
    static void draw(const HUDData& data);
};

} // namespace sky
