#include "Test.h"
#include "combat/WeaponSystem.h"
#include "combat/TargetingSystem.h"
#include "combat/Missile.h"
#include "combat/MissionSystem.h"
#include "ai/EnemyAI.h"
#include "ai/SteeringBehavior.h"
#include "world/TerrainSystem.h"
#include "ui/Radar.h"
#include "ui/HUD.h"
#include <algorithm>
#include <cmath>

using namespace sky;

int main() {
    Tests t;

    // =========================================================================
    // 1. WeaponSystem: Gun Cooldown, Pool Exhaustion, and Reuse
    // =========================================================================
    WeaponSystem weapons;
    t.check(weapons.activeBulletCount() == 0, "weapons start with empty active bullet pool");

    Vec3 muzzle{0, 100, 0};
    Vec3 fwd{0, 0, 1};

    // First shot succeeds
    bool shot1 = weapons.firePlayerGun(muzzle, fwd, 70.0);
    t.check(shot1 && weapons.activeBulletCount() == 1, "first gun shot succeeds and activates 1 bullet");
    t.check(weapons.gunCooldown() > 0.0, "gun cooldown is active after firing");

    // Immediate second shot fails due to cooldown
    bool shot2 = weapons.firePlayerGun(muzzle, fwd, 70.0);
    t.check(!shot2 && weapons.activeBulletCount() == 1, "immediate second shot blocked by gun cooldown");

    // Advance cooldown
    Aircraft dummyPlayer; dummyPlayer.reset();
    std::vector<Aircraft> dummyEnemies;
    weapons.update(weapons.gunCooldownTime(), nullptr, dummyPlayer, dummyEnemies);
    t.check(near(weapons.gunCooldown(), 0.0), "gun cooldown expires after cooldown duration");

    // Next shot succeeds
    bool shot3 = weapons.firePlayerGun(muzzle, fwd, 70.0);
    t.check(shot3 && weapons.activeBulletCount() == 2, "shot succeeds after cooldown expires");

    // Pool exhaustion: fire all 120 bullets
    weapons.reset();
    for (size_t i = 0; i < WeaponSystem::maxBullets; ++i) {
        weapons.resetGunCooldown();
        bool fired = weapons.firePlayerGun(muzzle, fwd, 70.0);
        if (!fired) break;
    }
    t.check(weapons.activeBulletCount() == WeaponSystem::maxBullets, "bullet pool filled to max capacity (120)");

    // 121st shot fails due to pool exhaustion
    weapons.resetGunCooldown();
    bool shotOverCapacity = weapons.firePlayerGun(muzzle, fwd, 70.0);
    t.check(!shotOverCapacity, "bullet firing dropped gracefully when pool is exhausted");

    // Pool reuse after bullets expire
    weapons.update(2.5, nullptr, dummyPlayer, dummyEnemies); // 2.5s exceeds 2.0s maxLifetime
    t.check(weapons.activeBulletCount() == 0, "bullets deactivate after lifetime expires");
    bool shotReused = weapons.firePlayerGun(muzzle, fwd, 70.0);
    t.check(shotReused && weapons.activeBulletCount() == 1, "reused bullet pool slot after expiration");

    // =========================================================================
    // 2. Bullet Hit and Miss vs Sphere Target
    // =========================================================================
    weapons.reset();
    Aircraft targetPlane;
    targetPlane.reset();
    targetPlane.transform.position = Vec3{0, 100, 80}; // 80m ahead
    targetPlane.health = 100.0;
    std::vector<Aircraft> targetList = {targetPlane};

    // Fire bullet directly at target plane
    weapons.firePlayerGun(Vec3{0, 100, 0}, Vec3{0, 0, 1}, 0.0);
    // In 0.2s at 600m/s, bullet travels 120m, passing through the target at 80m
    weapons.update(0.2, nullptr, dummyPlayer, targetList);

    t.check(targetList[0].health < 100.0, "bullet hits sphere target and deals damage");
    t.check(near(targetList[0].health, 100.0 - weapons.bulletDamage()), "exact bullet damage dealt (15 HP)");
    t.check(weapons.activeBulletCount() == 0, "bullet deactivates on target impact");

    // Fire bullet that misses target plane
    weapons.reset();
    targetList[0].health = 85.0;
    weapons.firePlayerGun(Vec3{0, 100, 0}, Vec3{1, 0, 0}, 0.0); // 90 deg away
    weapons.update(0.2, nullptr, dummyPlayer, targetList);

    t.check(near(targetList[0].health, 85.0), "missing bullet deals no damage");
    t.check(weapons.activeBulletCount() == 1, "missing bullet continues flying");

    // =========================================================================
    // 3. Bullet Stopped by Terrain
    // =========================================================================
    TerrainSystem terrain;
    terrain.initialize();
    weapons.reset();

    // Fire bullet downward toward terrain
    double gY = terrain.groundHeight(0.0, 0.0);
    weapons.firePlayerGun(Vec3{0.0, gY + 30.0, 0.0}, Vec3{0, -1, 0}, 0.0);
    weapons.update(0.1, &terrain, dummyPlayer, dummyEnemies); // 60m downward travel

    t.check(weapons.activeBulletCount() == 0, "bullet deactivates upon colliding with terrain");
    bool foundTerrainHit = false;
    for (const auto& ev : weapons.events()) {
        if (ev.type == CombatEventType::BulletHitTerrain) {
            foundTerrainHit = true;
            break;
        }
    }
    t.check(foundTerrainHit, "BulletHitTerrain event emitted upon terrain impact");

    // =========================================================================
    // 4. TargetingSystem: Cone and Range Locking
    // =========================================================================
    TargetingSystem targeting;
    targeting.reset();

    Vec3 playerPos{0, 120, 0};
    Vec3 playerFwd{0, 0, 1};

    // Target inside cone and range (500m ahead)
    std::vector<Vec3> targets = {Vec3{0, 120, 500}};
    std::vector<bool> alive = {true};

    targeting.update(0.1, playerPos, playerFwd, targets, alive);
    t.check(targeting.selectedTargetIndex() == 0, "auto-selects first living target");
    t.check(targeting.lockState() == LockState::Acquiring, "target inside cone starts Acquiring");
    t.check(targeting.lockProgress() > 0.0 && !targeting.hasValidLock(), "partial lock progress");

    // Advance acquire time to reach Locked state
    targeting.update(targeting.acquireTime(), playerPos, playerFwd, targets, alive);
    t.check(targeting.lockState() == LockState::Locked, "target inside cone reaches Locked after acquire time");
    t.check(targeting.hasValidLock(), "hasValidLock is true when Locked");

    // Target outside range (1800m ahead > 1200m maxLockRange)
    targets[0] = Vec3{0, 120, 1800};
    targeting.update(0.1, playerPos, playerFwd, targets, alive);
    t.check(targeting.lockState() == LockState::Searching, "target outside range rejected from locking");
    t.check(!targeting.hasValidLock(), "no lock when target is out of range");

    // Target outside view cone (90 degrees lateral at 500m)
    targets[0] = Vec3{500, 120, 0};
    targeting.update(0.1, playerPos, playerFwd, targets, alive);
    t.check(targeting.lockState() == LockState::Searching, "target outside view cone rejected from locking");

    // =========================================================================
    // 5. TargetingSystem: Tab Cycling Order
    // =========================================================================
    std::vector<Vec3> multiTargets = {
        Vec3{0, 120, 300},
        Vec3{100, 120, 300},
        Vec3{200, 120, 300},
        Vec3{300, 120, 300}
    };
    std::vector<bool> multiAlive = {true, true, true, true};

    targeting.reset();
    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 0, "Tab selects target 0");

    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 1, "Tab advances to target 1");

    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 2, "Tab advances to target 2");

    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 3, "Tab advances to target 3");

    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 0, "Tab wraps around to target 0");

    // Skip destroyed targets during Tab cycling
    multiAlive[1] = false; // Target 1 destroyed
    targeting.cycleTarget(multiTargets, multiAlive);
    t.check(targeting.selectedTargetIndex() == 2, "Tab skips destroyed target 1 and selects target 2");

    // =========================================================================
    // 6. Missile: Lock Requirement, Turn Rate Limit, and Interception
    // =========================================================================
    weapons.reset();
    // Launch without lock must fail
    bool missNoLock = weapons.firePlayerMissile(playerPos, playerFwd, 0, false, 70.0);
    t.check(!missNoLock, "missile launch rejected without valid lock");

    // Launch with lock succeeds
    bool missWithLock = weapons.firePlayerMissile(playerPos, playerFwd, 0, true, 70.0);
    t.check(missWithLock && weapons.activeMissileCount() == 1, "missile launches with valid lock");

    // Missile turn rate test: homing on target 90 degrees offset
    Missile mTurnTest;
    mTurnTest.launch(Vec3{0, 100, 0}, Vec3{0, 0, 1}, 0, 160.0);
    Vec3 offsetTarget{500, 100, 100}; // Far to the right
    std::vector<CombatEvent> mEvents;

    mTurnTest.update(0.1, offsetTarget, true, nullptr, mEvents);
    double turnedAngle = std::acos(std::clamp(dot(mTurnTest.forward(), Vec3{0, 0, 1}), -1.0, 1.0));

    // Turn in 0.1s cannot exceed turnRateLimit * 0.1 + epsilon
    t.check(turnedAngle <= mTurnTest.turnRateLimit() * 0.1 + 1e-4,
            "missile angular velocity strictly capped by turnRateLimit");
    t.check(turnedAngle > 0.0, "missile turns toward target direction");

    // Missile intercepting a straight-flying target plane
    weapons.reset();
    Aircraft movingTarget;
    movingTarget.reset();
    movingTarget.transform.position = Vec3{0, 100, 300}; // 300m ahead
    movingTarget.speed = 70.0;
    movingTarget.health = 100.0;
    std::vector<Aircraft> movingList = {movingTarget};

    weapons.firePlayerMissile(Vec3{0, 100, 0}, Vec3{0, 0, 1}, 0, true, 70.0);
    t.check(weapons.activeMissileCount() == 1, "interception test missile active");

    // Simulate flight until interception (at relative speed ~140m/s, ~2.5s)
    bool intercepted = false;
    for (int frame = 0; frame < 240; ++frame) {
        movingList[0].transform.position.z += movingList[0].speed * (1.0 / 60.0);
        weapons.update(1.0 / 60.0, nullptr, dummyPlayer, movingList);
        if (movingList[0].health < 100.0) {
            intercepted = true;
            break;
        }
    }
    t.check(intercepted, "homing missile successfully intercepts moving aircraft target");
    t.check(movingList[0].health <= 20.0, "missile detonation deals 80 damage to target");

    // =========================================================================
    // 7. EnemyAI: FSM State Transitions
    // =========================================================================
    EnemyAI enemy(0, Vec3{0, 120, 1200}, Vec3{0, 0, -1});
    Aircraft testPlayer; testPlayer.reset();
    testPlayer.transform.position = Vec3{0, 120, 0};

    // 1. Far away -> Patrol
    enemy.update(0.1, testPlayer, false, false, nullptr, nullptr);
    t.check(enemy.state() == AIState::Patrol, "enemy enters Patrol when player is out of range");

    // 2. Detection range (600m <= 850m) -> Chase
    testPlayer.transform.position = Vec3{0, 120, 600};
    enemy.update(0.1, testPlayer, false, false, nullptr, nullptr);
    t.check(enemy.state() == AIState::Chase, "enemy transitions to Chase when player in detection range");

    // 3. Attack range (250m <= 400m) and in front cone -> Attack
    enemy.aircraft().transform.position = Vec3{0, 120, 250};
    enemy.aircraft().transform.orientation = Quat::axisAngle(Vec3{0, 1, 0}, pi); // facing player
    testPlayer.transform.position = Vec3{0, 120, 0};
    enemy.update(0.1, testPlayer, false, false, nullptr, nullptr);
    t.check(enemy.state() == AIState::Attack, "enemy transitions to Attack when in range and facing player");

    // 4. Locked by player -> Evade
    enemy.update(0.1, testPlayer, true, false, nullptr, nullptr);
    t.check(enemy.state() == AIState::Evade, "enemy transitions to Evade when locked by player");

    // 5. Incoming missile -> Evade
    enemy.update(0.1, testPlayer, false, true, nullptr, nullptr);
    t.check(enemy.state() == AIState::Evade, "enemy transitions to Evade when missile is incoming");

    // 6. Low health (HP <= 25) -> Retreat
    enemy.aircraft().health = 20.0;
    enemy.update(0.1, testPlayer, false, false, nullptr, nullptr);
    t.check(enemy.state() == AIState::Retreat, "enemy transitions to Retreat when health <= 25 HP");

    // =========================================================================
    // 8. Long Simulation: Enemy Never Flies Below Terrain
    // =========================================================================
    EnemyAI lowEnemy(1, Vec3{0, 60, -400}, Vec3{0, 0, 1}); // Valley sector
    bool remainedAboveGround = true;

    for (int step = 0; step < 600; ++step) { // 10 seconds of simulation
        lowEnemy.update(1.0 / 60.0, testPlayer, false, false, &terrain, nullptr);
        double curY = lowEnemy.aircraft().transform.position.y;
        double groundY = terrain.groundHeight(lowEnemy.aircraft().transform.position.x,
                                             lowEnemy.aircraft().transform.position.z);
        if (curY < groundY - 1e-4) {
            remainedAboveGround = false;
            break;
        }
    }
    t.check(remainedAboveGround, "enemy AI respects terrain avoidance and never clips below ground in long simulation");

    // =========================================================================
    // 9. MissionSystem: Active, Cleared, Failed, and Reset
    // =========================================================================
    MissionSystem mission(4);
    std::vector<EnemyAI> missionEnemies;
    missionEnemies.emplace_back(0, Vec3{100, 120, 200});
    missionEnemies.emplace_back(1, Vec3{200, 120, 200});
    missionEnemies.emplace_back(2, Vec3{300, 120, 200});
    missionEnemies.emplace_back(3, Vec3{400, 120, 200});

    mission.update(0.1, testPlayer, missionEnemies);
    t.check(mission.isActive(), "mission starts Active");
    t.check(mission.enemiesRemaining() == 4, "4 enemies remaining initially");

    // Destroy 3 enemies
    missionEnemies[0].aircraft().health = 0.0;
    missionEnemies[1].aircraft().health = 0.0;
    missionEnemies[2].aircraft().health = 0.0;
    mission.update(0.1, testPlayer, missionEnemies);
    t.check(mission.isActive() && mission.enemiesRemaining() == 1, "mission remains active with 1 target left");

    // Destroy last enemy -> Mission Cleared
    missionEnemies[3].aircraft().health = 0.0;
    mission.update(0.1, testPlayer, missionEnemies);
    t.check(mission.isCleared(), "mission transitions to Cleared when all enemies are destroyed");

    // Reset restores Active state and enemy count
    mission.reset(4);
    t.check(mission.isActive() && mission.enemiesRemaining() == 4, "mission reset restores Active status");

    // Player destroyed -> Mission Failed
    testPlayer.health = 0.0;
    testPlayer.crashed = true;
    mission.update(0.1, testPlayer, missionEnemies);
    t.check(mission.isFailed(), "mission transitions to Failed when player aircraft is destroyed");

    // =========================================================================
    // 10. Radar: Relative Heading Projections and Blips
    // =========================================================================
    Aircraft radarPlayer; radarPlayer.reset();
    radarPlayer.transform.position = Vec3{0, 120, 0}; // facing +Z

    std::vector<EnemyAI> radarEnemies;
    radarEnemies.emplace_back(0, Vec3{0, 120, 750});   // Ahead at 750m
    radarEnemies.emplace_back(1, Vec3{750, 120, 0});   // Right at 750m

    auto rData = Radar::createData(radarPlayer, radarEnemies, 0, true, 1500.0);
    t.check(rData.blips.size() == 2, "radar creates blips for living enemies");

    // Enemy 0 (ahead) has relX ~ 0 and relY = 750 / 1500 = 0.5
    t.check(near(rData.blips[0].relX, 0.0, 1e-3) && near(rData.blips[0].relY, 0.5, 1e-3),
            "radar blip ahead correctly projected (+Y rel)");
    t.check(rData.blips[0].isLocked, "targeted and locked enemy has isLocked flag");

    // Enemy 1 (right) has relX = 0.5 and relY = 0.0
    t.check(near(rData.blips[1].relX, 0.5, 1e-3) && near(rData.blips[1].relY, 0.0, 1e-3),
            "radar blip to the right correctly projected (+X rel)");

    return t.finish();
}
