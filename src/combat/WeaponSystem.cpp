#include "WeaponSystem.h"
#include "world/HeightProvider.h"
#include <algorithm>
#include <cmath>

namespace sky {

WeaponSystem::WeaponSystem() {
    reset();
}

void WeaponSystem::reset() {
    for (auto& b : bullets_) {
        b.active = false;
    }
    for (auto& m : missiles_) {
        m.deactivate();
    }
    events_.clear();
    gunCooldown_ = 0.0;
    missileCooldown_ = 0.0;
}

size_t WeaponSystem::activeBulletCount() const {
    size_t count = 0;
    for (const auto& b : bullets_) {
        if (b.active) ++count;
    }
    return count;
}

size_t WeaponSystem::activeMissileCount() const {
    size_t count = 0;
    for (const auto& m : missiles_) {
        if (m.isActive()) ++count;
    }
    return count;
}

bool WeaponSystem::firePlayerGun(const Vec3& muzzlePos, const Vec3& muzzleDir, double aircraftSpeed) {
    if (gunCooldown_ > 0.0) return false;

    for (auto& b : bullets_) {
        if (!b.active) {
            b.position = muzzlePos;
            b.velocity = normalized(muzzleDir) * (aircraftSpeed + bulletSpeed_);
            b.lifetime = 0.0;
            b.maxLifetime = bulletLifetime_;
            b.damage = bulletDamage_;
            b.fromPlayer = true;
            b.shooterId = -1;
            b.active = true;

            gunCooldown_ = gunCooldownTime_;
            events_.push_back({CombatEventType::BulletFired, muzzlePos, -1, 0.0});
            return true;
        }
    }
    // Pool exhausted
    return false;
}

bool WeaponSystem::fireEnemyGun(int enemyId, const Vec3& muzzlePos, const Vec3& muzzleDir, double aircraftSpeed) {
    for (auto& b : bullets_) {
        if (!b.active) {
            b.position = muzzlePos;
            b.velocity = normalized(muzzleDir) * (aircraftSpeed + bulletSpeed_);
            b.lifetime = 0.0;
            b.maxLifetime = bulletLifetime_;
            b.damage = 10.0; // Enemy deals 10 damage per hit
            b.fromPlayer = false;
            b.shooterId = enemyId;
            b.active = true;

            events_.push_back({CombatEventType::BulletFired, muzzlePos, enemyId, 0.0});
            return true;
        }
    }
    return false;
}

bool WeaponSystem::firePlayerMissile(const Vec3& startPos, const Vec3& startDir, int targetId, bool hasLock, double aircraftSpeed) {
    if (!hasLock || missileCooldown_ > 0.0 || targetId < 0) return false;

    for (auto& m : missiles_) {
        if (!m.isActive()) {
            m.launch(startPos, startDir, targetId, aircraftSpeed + 50.0);
            missileCooldown_ = missileCooldownTime_;
            events_.push_back({CombatEventType::MissileLaunched, startPos, targetId, 0.0});
            return true;
        }
    }
    return false;
}

void WeaponSystem::update(double dt, const IHeightProvider* terrain, Aircraft& player, std::vector<Aircraft>& enemies) {
    if (gunCooldown_ > 0.0) gunCooldown_ -= dt;
    if (missileCooldown_ > 0.0) missileCooldown_ -= dt;

    if (dt <= 0.0) return;

    // 1. Advance bullets in anti-tunneling substeps with swept-segment intersection
    constexpr int numSubsteps = 4;
    double subDt = dt / double(numSubsteps);

    for (int step = 0; step < numSubsteps; ++step) {
        for (auto& b : bullets_) {
            if (!b.active) continue;

            Vec3 p0 = b.position;
            Vec3 p1 = p0 + b.velocity * subDt;
            Vec3 seg = p1 - p0;
            double segLen2 = dot(seg, seg);

            // Target collision check (swept segment-to-sphere test)
            if (b.fromPlayer) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    auto& e = enemies[i];
                    if (e.health <= 0.0) continue;

                    Vec3 c = e.transform.position;
                    double t = (segLen2 > 1e-12) ? std::clamp(dot(c - p0, seg) / segLen2, 0.0, 1.0) : 0.0;
                    Vec3 closest = p0 + seg * t;

                    if (length(closest - c) <= aircraftHitRadius_) {
                        e.health -= b.damage;
                        events_.push_back({CombatEventType::BulletHitAircraft, closest, int(i), b.damage});
                        if (e.health <= 0.0) {
                            e.health = 0.0;
                            e.crashed = true;
                            events_.push_back({CombatEventType::AircraftDestroyed, e.transform.position, int(i), 0.0});
                        }
                        b.active = false;
                        break;
                    }
                }
            } else {
                if (player.health > 0.0) {
                    Vec3 c = player.transform.position;
                    double t = (segLen2 > 1e-12) ? std::clamp(dot(c - p0, seg) / segLen2, 0.0, 1.0) : 0.0;
                    Vec3 closest = p0 + seg * t;

                    if (length(closest - c) <= aircraftHitRadius_) {
                        player.health -= b.damage;
                        events_.push_back({CombatEventType::BulletHitAircraft, closest, -1, b.damage});
                        if (player.health <= 0.0) {
                            player.health = 0.0;
                            player.crashed = true;
                            events_.push_back({CombatEventType::AircraftDestroyed, player.transform.position, -1, 0.0});
                        }
                        b.active = false;
                    }
                }
            }

            if (!b.active) continue;

            // Terrain collision check
            if (terrain && p1.y <= terrain->groundHeight(p1.x, p1.z)) {
                b.active = false;
                events_.push_back({CombatEventType::BulletHitTerrain, p1, -1, 0.0});
                continue;
            }

            // Step advance
            b.position = p1;
        }
    }

    // Bullet lifetime check
    for (auto& b : bullets_) {
        if (!b.active) continue;
        b.lifetime += dt;
        if (b.lifetime >= b.maxLifetime) {
            b.active = false;
        }
    }

    // 2. Advance missiles
    for (auto& m : missiles_) {
        if (!m.isActive()) continue;

        int tid = m.targetId();
        bool targetValid = (tid >= 0 && tid < int(enemies.size()) && enemies[tid].health > 0.0);
        Vec3 targetPos = targetValid ? enemies[tid].transform.position : Vec3{};

        size_t prevEventCount = events_.size();
        m.update(dt, targetPos, targetValid, terrain, events_);

        // Check if missile generated a hit event in this step
        for (size_t ev = prevEventCount; ev < events_.size(); ++ev) {
            const auto& e = events_[ev];
            if (e.type == CombatEventType::MissileDetonated && e.damage > 0.0 && targetValid) {
                enemies[tid].health -= e.damage;
                if (enemies[tid].health <= 0.0) {
                    enemies[tid].health = 0.0;
                    enemies[tid].crashed = true;
                    events_.push_back({CombatEventType::AircraftDestroyed, enemies[tid].transform.position, tid, 0.0});
                }
            }
        }
    }
}

} // namespace sky
