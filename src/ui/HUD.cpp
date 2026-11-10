#include "HUD.h"
#include <cstdio>
#include <cstdint>

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

double HUD::toKmh(double speedMs) {
    return speedMs * 3.6;
}

FlightAssistState HUD::evaluateAssistState(const Aircraft& a) {
    if (a.crashed || a.health <= 0.0) return FlightAssistState::Crashed;
    if (a.terrainWarning) return FlightAssistState::LowClearanceWarning;
    if (a.terrainAssist) return FlightAssistState::AssistEngaged;
    return FlightAssistState::Normal;
}

HUDData HUD::createData(const Aircraft& a, double fps, std::string_view biomeName,
                        size_t targetsRemaining, int selectedTargetIndex,
                        double targetDistance, LockState lockState,
                        MissionStatus missionStatus) {
    HUDData d;
    d.speedKmh = toKmh(a.speed);
    d.altitudeM = a.transform.position.y;
    d.clearanceM = a.clearance;
    d.assistState = evaluateAssistState(a);
    d.fps = fps;
    d.biomeString = std::string(biomeName);

    d.playerHealth = a.health;
    d.targetsRemaining = targetsRemaining;
    d.selectedTargetIndex = selectedTargetIndex;
    d.targetDistance = targetDistance;
    d.lockState = lockState;
    d.missionStatus = missionStatus;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.1f km/h", d.speedKmh);
    d.speedString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f m", d.altitudeM);
    d.altitudeString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f m", d.clearanceM);
    d.clearanceString = buf;

    std::snprintf(buf, sizeof(buf), "%.1f FPS", d.fps);
    d.fpsString = buf;

    std::snprintf(buf, sizeof(buf), "HP: %.0f%%", std::max(0.0, d.playerHealth));
    d.healthString = buf;

    std::snprintf(buf, sizeof(buf), "TARGETS: %zu", d.targetsRemaining);
    d.targetsString = buf;

    if (d.selectedTargetIndex >= 0) {
        const char* lockStr = "[SEARCHING]";
        if (d.lockState == LockState::Acquiring) lockStr = "[LOCKING...]";
        else if (d.lockState == LockState::Locked) lockStr = "[★ LOCKED - FIRE F ★]";

        std::snprintf(buf, sizeof(buf), "TGT: BANDIT-%02d | %0.0fm | %s",
                      d.selectedTargetIndex + 1, d.targetDistance, lockStr);
    } else {
        std::snprintf(buf, sizeof(buf), "TGT: NO TARGET (TAB TO CYCLE)");
    }
    d.targetInfoString = buf;

    if (d.missionStatus == MissionStatus::Cleared) {
        d.missionBannerString = "[★ MISSION COMPLETE - ALL HOSTILES DESTROYED - PRESS R TO RESTART ★]";
    } else if (d.missionStatus == MissionStatus::Failed) {
        d.missionBannerString = "[☠ MISSION FAILED - AIRCRAFT DESTROYED - PRESS R TO RESPAWN ☠]";
    } else {
        d.missionBannerString = "";
    }

    switch (d.assistState) {
        case FlightAssistState::Crashed:
            d.assistStateString = "[DESTROYED / CRASHED - PRESS R TO RESPAWN]";
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
    // Line 1: Primary flight telemetry
    DrawFormatString(16, 16, GetColor(255, 255, 255),
                     L"M5 Combat | Speed: %S | Alt: %S | Clearance: %S | %S",
                     data.speedString.c_str(),
                     data.altitudeString.c_str(),
                     data.clearanceString.c_str(),
                     data.fpsString.c_str());

    // Line 2: Combat Status (HP & Targets remaining)
    uint32_t hpColor = (data.playerHealth > 50.0) ? GetColor(120, 255, 120) :
                       (data.playerHealth > 25.0) ? GetColor(255, 220, 80) : GetColor(255, 70, 70);

    DrawFormatString(16, 40, hpColor, L"%S", data.healthString.c_str());
    DrawFormatString(160, 40, GetColor(255, 200, 100), L"%S", data.targetsString.c_str());

    // Line 3: Target info & Lock status
    uint32_t lockColor = (data.lockState == LockState::Locked) ? GetColor(255, 240, 50) :
                         (data.lockState == LockState::Acquiring) ? GetColor(255, 180, 50) :
                         GetColor(180, 220, 240);
    DrawFormatString(16, 64, lockColor, L"%S", data.targetInfoString.c_str());

    // Line 4: Controls help
    DrawString(16, 88, L"Mouse Steer | LMB Gun | Hold RMB Lock | F Missile | Tab Target | A/D Roll | Shift/Ctrl Speed | C Cam | R Reset | Esc Pause",
               GetColor(220, 230, 240));

    // Flight assist status
    uint32_t stateColor = GetColor(160, 240, 160);
    if (data.assistState == FlightAssistState::Crashed) {
        stateColor = GetColor(255, 80, 80);
    } else if (data.assistState == FlightAssistState::LowClearanceWarning) {
        stateColor = GetColor(255, 60, 60);
    } else if (data.assistState == FlightAssistState::AssistEngaged) {
        stateColor = GetColor(255, 230, 80);
    }
    DrawFormatString(16, 112, stateColor, L"%S", data.assistStateString.c_str());

    // Mission state banner (centered prominent alert)
    if (!data.missionBannerString.empty()) {
        uint32_t bannerColor = (data.missionStatus == MissionStatus::Cleared) ?
                               GetColor(100, 255, 120) : GetColor(255, 70, 70);
        DrawFormatString(320, 320, bannerColor, L"%S", data.missionBannerString.c_str());
    }
#else
    (void)data;
#endif
}

} // namespace sky
