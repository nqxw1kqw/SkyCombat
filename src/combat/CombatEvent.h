#pragma once
#include "core/Math.h"

namespace sky {

enum class CombatEventType {
    BulletFired,
    BulletHitTerrain,
    BulletHitAircraft,
    MissileLaunched,
    MissileDetonated,
    AircraftDestroyed
};

struct CombatEvent {
    CombatEventType type{CombatEventType::BulletFired};
    Vec3 position{};
    int targetId{-1};
    double damage{0.0};
};

} // namespace sky
