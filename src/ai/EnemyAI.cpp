#include "EnemyAI.h"
#include "SteeringBehavior.h"
#include "combat/WeaponSystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

std::string_view EnemyAI::stateName(AIState s) {
    switch (s) {
        case AIState::Patrol:  return "PATROL";
        case AIState::Chase:   return "CHASE";
        case AIState::Attack:  return "ATTACK";
        case AIState::Evade:   return "EVADE";
        case AIState::Retreat: return "RETREAT";
        default:               return "UNKNOWN";
    }
}

EnemyAI::EnemyAI(int id, const Vec3& spawnPos, const Vec3& forward)
    : id_(id), spawnPosition_(spawnPos), patrolCenter_(spawnPos) {
    reset(spawnPos, forward);
}

void EnemyAI::reset(const Vec3& spawnPos, const Vec3& forward) {
    aircraft_.reset();
    aircraft_.transform.position = spawnPos;

    Vec3 fwd = normalized(forward);
    Vec3 axis = cross(Vec3{0, 0, 1}, fwd);
    if (length(axis) < 1e-6) {
        if (dot(Vec3{0, 0, 1}, fwd) < 0.0) {
            aircraft_.transform.orientation = Quat::axisAngle(Vec3{0, 1, 0}, pi);
        } else {
            aircraft_.transform.orientation = Quat{};
        }
    } else {
        double angle = std::acos(std::clamp(dot(Vec3{0, 0, 1}, fwd), -1.0, 1.0));
        aircraft_.transform.orientation = Quat::axisAngle(axis, angle);
    }

    aircraft_.speed = 65.0;
    aircraft_.health = 100.0;
    aircraft_.crashed = false;

    state_ = AIState::Patrol;
    patrolCenter_ = spawnPos;
    patrolAngle_ = double(id_) * 1.57;
    gunTimer_ = 0.0;
    evadeTimer_ = 0.0;
    lastHealth_ = 100.0;
}

void EnemyAI::update(double dt, const Aircraft& player, bool playerLockedMe, bool missileHomingOnMe,
                     const IHeightProvider* terrain, WeaponSystem* weapons) {
    if (!isAlive() || dt <= 0.0) return;

    if (gunTimer_ > 0.0) gunTimer_ -= dt;
    if (evadeTimer_ > 0.0) evadeTimer_ -= dt;

    // Detect if damaged recently
    bool tookDamage = (aircraft_.health < lastHealth_ - 1.0);
    lastHealth_ = aircraft_.health;
    if (tookDamage) {
        evadeTimer_ = 1.5; // Evade for 1.5s after hit
    }

    Vec3 pos = aircraft_.transform.position;
    Vec3 playerPos = player.transform.position;
    Vec3 toPlayer = playerPos - pos;
    double distToPlayer = length(toPlayer);
    Vec3 fwd = aircraft_.transform.forward();

    double angleToPlayer = 0.0;
    if (distToPlayer > 1e-4) {
        double dotVal = std::clamp(dot(fwd, normalized(toPlayer)), -1.0, 1.0);
        angleToPlayer = std::acos(dotVal);
    }

    // =========================================================================
    // FSM State Transitions
    // =========================================================================
    if (aircraft_.health <= retreatHealthThreshold_) {
        state_ = AIState::Retreat;
    } else if (playerLockedMe || missileHomingOnMe || evadeTimer_ > 0.0) {
        state_ = AIState::Evade;
    } else if (distToPlayer <= attackRange_ && angleToPlayer <= attackAngle_) {
        state_ = AIState::Attack;
    } else if (distToPlayer <= detectionRange_) {
        state_ = AIState::Chase;
    } else {
        state_ = AIState::Patrol;
    }

    // =========================================================================
    // Steering & Actions per State
    // =========================================================================
    FlightInput input{};

    switch (state_) {
        case AIState::Retreat: {
            // Flee away from player, climb to high altitude
            input = SteeringBehavior::flee(aircraft_, playerPos, 90.0, terrain);
            break;
        }

        case AIState::Evade: {
            // Break hard perpendicular to player's line of fire
            input = SteeringBehavior::evade(aircraft_, playerPos, player.transform.forward(), 110.0, terrain);
            break;
        }

        case AIState::Attack: {
            // Seek player, match cruise/attack speed, fire weapon
            input = SteeringBehavior::seek(aircraft_, playerPos, 70.0, terrain);
            if (weapons && gunTimer_ <= 0.0 && distToPlayer <= attackRange_ && angleToPlayer <= attackAngle_) {
                weapons->fireEnemyGun(id_, pos + fwd * 4.0, fwd, aircraft_.speed);
                gunTimer_ = gunCooldown_;
            }
            break;
        }

        case AIState::Chase: {
            // Intercept player at high cruise speed
            input = SteeringBehavior::seek(aircraft_, playerPos, 85.0, terrain);
            break;
        }

        case AIState::Patrol:
        default: {
            // Circular loiter patrol around patrol center
            patrolAngle_ += 0.25 * dt;
            Vec3 patrolTarget{
                patrolCenter_.x + std::cos(patrolAngle_) * 250.0,
                patrolCenter_.y,
                patrolCenter_.z + std::sin(patrolAngle_) * 250.0
            };
            input = SteeringBehavior::seek(aircraft_, patrolTarget, 60.0, terrain);
            break;
        }
    }

    // Advance aircraft using FlightController
    controller_.update(aircraft_, input, dt, terrain);
}

} // namespace sky
