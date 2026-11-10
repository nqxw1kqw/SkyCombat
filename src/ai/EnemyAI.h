#pragma once
#include "aircraft/Aircraft.h"
#include "aircraft/FlightController.h"
#include <vector>
#include <string_view>

namespace sky {

class IHeightProvider;
class WeaponSystem;

enum class AIState {
    Patrol,
    Chase,
    Attack,
    Evade,
    Retreat
};

class EnemyAI {
public:
    EnemyAI(int id, const Vec3& spawnPos, const Vec3& forward = {0, 0, 1});

    void reset(const Vec3& spawnPos, const Vec3& forward = {0, 0, 1});

    void update(double dt, const Aircraft& player, bool playerLockedMe, bool missileHomingOnMe,
                const IHeightProvider* terrain, WeaponSystem* weapons);

    int id() const { return id_; }
    AIState state() const { return state_; }
    void setState(AIState s) { state_ = s; }

    const Aircraft& aircraft() const { return aircraft_; }
    Aircraft& aircraft() { return aircraft_; }

    bool isAlive() const { return aircraft_.health > 0.0 && !aircraft_.crashed; }

    static std::string_view stateName(AIState state);

    double detectionRange() const { return detectionRange_; }
    double attackRange() const { return attackRange_; }
    double attackAngle() const { return attackAngle_; }
    double retreatHealthThreshold() const { return retreatHealthThreshold_; }

private:
    int id_{0};
    AIState state_{AIState::Patrol};
    Aircraft aircraft_{};
    FlightController controller_{};

    Vec3 spawnPosition_{};
    Vec3 patrolCenter_{};
    double patrolAngle_{0.0};

    double detectionRange_{850.0};          // Distance to switch from Patrol to Chase
    double attackRange_{400.0};             // Distance to switch from Chase to Attack
    double attackAngle_{radians(25.0)};     // Front cone half-angle to fire
    double retreatHealthThreshold_{25.0};   // HP threshold to switch to Retreat

    double gunTimer_{0.0};
    double gunCooldown_{0.20};              // 5 shots/sec
    double evadeTimer_{0.0};
    double lastHealth_{100.0};
};

} // namespace sky
