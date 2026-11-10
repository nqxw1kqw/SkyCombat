#include "HUD.h"
#include <cstdio>

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

double HUD::toKmh(double speedMs) {
    return speedMs * 3.6;
}

FlightAssistState HUD::evaluateAssistState(const Aircraft& a) {
    if (a.crashed) return FlightAssistState::Crashed;
    if (a.terrainWarning) return FlightAssistState::LowClearanceWarning;
    if (a.terrainAssist) return FlightAssistState::AssistEngaged;
    return FlightAssistState::Normal;
}

HUDData HUD::createData(const Aircraft& a, double fps, std::string_view biomeName) {
    HUDData d;
    d.speedKmh = toKmh(a.speed);
    d.altitudeM = a.transform.position.y;
    d.clearanceM = a.clearance;
    d.assistState = evaluateAssistState(a);
    d.fps = fps;
    d.biomeString = std::string(biomeName);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.1f km/h", d.speedKmh);
    d.speedString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f m", d.altitudeM);
    d.altitudeString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f m", d.clearanceM);
    d.clearanceString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f FPS", d.fps);
    d.fpsString = buf;

    switch (d.assistState) {
        case FlightAssistState::Crashed:
            d.assistStateString = "[CRASHED - PRESS R TO RESPAWN]";
            break;
        case FlightAssistState::LowClearanceWarning:
            d.assistStateString = "[WARNING: LOW CLEARANCE (<8m)]";
            break;
        case FlightAssistState::AssistEngaged:
            d.assistStateString = "[TERRAIN ASSIST ACTIVE (8-25m)]";
            break;
        case FlightAssistState::Normal:
        default:
            d.assistStateString = "[CLEARANCE NORMAL (>25m)]";
            break;
    }

    return d;
}

void HUD::draw(const HUDData& data) {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    DrawFormatString(16, 16, GetColor(255, 255, 255),
                     L"M4 | Speed: %S | Alt: %S | Clearance: %S | %S",
                     data.speedString.c_str(),
                     data.altitudeString.c_str(),
                     data.clearanceString.c_str(),
                     data.fpsString.c_str());

    DrawFormatString(16, 40, GetColor(240, 240, 255),
                     L"Biome: %S | Environment Art: Batched Vegetation & Low-poly Cloud Clusters",
                     data.biomeString.c_str());

    DrawString(16, 64, L"W/S pitch | A/D turn | Shift/Ctrl throttle | R reset | Esc quit",
               GetColor(255, 255, 255));

    uint32_t stateColor = GetColor(160, 240, 160);
    if (data.assistState == FlightAssistState::Crashed) {
        stateColor = GetColor(255, 80, 80);
    } else if (data.assistState == FlightAssistState::LowClearanceWarning) {
        stateColor = GetColor(255, 60, 60);
    } else if (data.assistState == FlightAssistState::AssistEngaged) {
        stateColor = GetColor(255, 230, 80);
    }

    DrawFormatString(16, 88, stateColor, L"%S", data.assistStateString.c_str());
#else
    (void)data;
#endif
}

} // namespace sky
