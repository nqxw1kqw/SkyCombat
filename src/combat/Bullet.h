#pragma once
#include "core/Math.h"

namespace sky {

struct Bullet {
    Vec3 position{};
    Vec3 velocity{};
    double lifetime{0.0};
    double maxLifetime{2.0}; // seconds
    double damage{15.0};
    bool active{false};
    bool fromPlayer{true};
    int shooterId{-1}; // -1 for player, >= 0 for enemy index
};

} // namespace sky
