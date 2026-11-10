#pragma once
#include "aircraft/Aircraft.h"
#include "combat/TargetingSystem.h"
#include "combat/MissionSystem.h"
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

    // M5 Combat additions
    double playerHealth{100.0};
    std::string healthString;
    size_t targetsRemaining{0};
    std::string targetsString;
    int selectedTargetIndex{-1};
    double targetDistance{0.0};
    LockState lockState{LockState::NoTarget};
    std::string targetInfoString;
    MissionStatus missionStatus{MissionStatus::Active};
    std::string missionBannerString;
};

class HUD {
public:
    static double toKmh(double speedMs);
    static FlightAssistState evaluateAssistState(const Aircraft& a);

    static HUDData createData(const Aircraft& a, double fps, std::string_view biomeName,
                             size_t targetsRemaining = 0, int selectedTargetIndex = -1,
                             double targetDistance = 0.0, LockState lockState = LockState::NoTarget,
                             MissionStatus missionStatus = MissionStatus::Active);

    // DXLib rendering layer: only draws precomputed strings from HUDData
    static void draw(const HUDData& data);
};

} // namespace sky
