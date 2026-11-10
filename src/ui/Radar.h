#pragma once
#include "aircraft/Aircraft.h"
#include "ai/EnemyAI.h"
#include <vector>

namespace sky {

struct RadarBlip {
    double relX{0.0}; // [-1.0, 1.0], +X is right of player heading
    double relY{0.0}; // [-1.0, 1.0], +Y is ahead of player heading
    double distance{0.0};
    bool isLocked{false};
    bool isTargeted{false};
    bool isAlive{true};
    int id{-1};
};

struct RadarData {
    std::vector<RadarBlip> blips;
    double range{1500.0}; // Detection range in meters
    int screenX{1160};
    int screenY{600};
    int radius{75};
};

class Radar {
public:
    static RadarData createData(const Aircraft& player, const std::vector<EnemyAI>& enemies,
                                int targetedIndex, bool hasLock, double range = 1500.0);

    // DXLib rendering layer: only draws the radar HUD graphic from RadarData
    static void draw(const RadarData& data);
};

} // namespace sky
