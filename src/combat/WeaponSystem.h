#pragma once
#include "Bullet.h"
#include "Missile.h"
#include "CombatEvent.h"
#include "aircraft/Aircraft.h"
#include <array>
#include <vector>

namespace sky {

class IHeightProvider;

class WeaponSystem {
public:
    static constexpr size_t maxBullets = 120;
    static constexpr size_t maxMissiles = 8;

    WeaponSystem();

    void reset();

    // Fire weapons
    bool firePlayerGun(const Vec3& muzzlePos, const Vec3& muzzleDir, double aircraftSpeed);
    bool fireEnemyGun(int enemyId, const Vec3& muzzlePos, const Vec3& muzzleDir, double aircraftSpeed);
    bool firePlayerMissile(const Vec3& startPos, const Vec3& startDir, int targetId, bool hasLock, double aircraftSpeed);

    // Updates bullets, missiles, collisions, and damage
    void update(double dt, const IHeightProvider* terrain, Aircraft& player, std::vector<Aircraft>& enemies);

    const std::array<Bullet, maxBullets>& bullets() const { return bullets_; }
    const std::array<Missile, maxMissiles>& missiles() const { return missiles_; }
    const std::vector<CombatEvent>& events() const { return events_; }
    void clearEvents() { events_.clear(); }

    double gunCooldown() const { return gunCooldown_; }
    double missileCooldown() const { return missileCooldown_; }
    double gunCooldownTime() const { return gunCooldownTime_; }
    double missileCooldownTime() const { return missileCooldownTime_; }
    double bulletSpeed() const { return bulletSpeed_; }
    double bulletDamage() const { return bulletDamage_; }
    double aircraftHitRadius() const { return aircraftHitRadius_; }

    void setGunCooldownTime(double t) { gunCooldownTime_ = t; }
    void setMissileCooldownTime(double t) { missileCooldownTime_ = t; }
    void setBulletSpeed(double s) { bulletSpeed_ = s; }
    void setAircraftHitRadius(double r) { aircraftHitRadius_ = r; }
    void resetGunCooldown() { gunCooldown_ = 0.0; }

    size_t activeBulletCount() const;
    size_t activeMissileCount() const;

private:
    std::array<Bullet, maxBullets> bullets_{};
    std::array<Missile, maxMissiles> missiles_{};
    std::vector<CombatEvent> events_;

    double gunCooldown_{0.0};
    double gunCooldownTime_{0.08};       // ~12.5 shots/second
    double missileCooldown_{0.0};
    double missileCooldownTime_{1.20};   // 1.2s between missile launches

    double bulletSpeed_{600.0};          // 600 m/s
    double bulletDamage_{15.0};          // 15 HP per hit
    double bulletLifetime_{2.0};         // 2 seconds
    double aircraftHitRadius_{3.5};      // 3.5m collision sphere
};

} // namespace sky
