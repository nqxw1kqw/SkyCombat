#include "Game.h"
#include "Time.h"
#include "Input.h"
#include "aircraft/FlightController.h"
#include "camera/ChaseCamera.h"
#include "rendering/AircraftRenderer.h"
#include "rendering/TerrainRenderer.h"
#include "world/TerrainSystem.h"
#include "world/SkySystem.h"
#include "world/VegetationSystem.h"
#include "world/CloudSystem.h"
#include "effects/SpeedEffects.h"
#include "audio/AudioModel.h"
#include "combat/WeaponSystem.h"
#include "combat/TargetingSystem.h"
#include "combat/MissionSystem.h"
#include "ai/EnemyAI.h"
#include "ui/HUD.h"
#include "ui/Radar.h"
#include <DxLib.h>
#include <cmath>
#include <vector>

namespace sky {

namespace {

VECTOR vector(Vec3 v) { return VGet(float(v.x), float(v.y), float(v.z)); }

KeyboardState poll() {
    // Inactive windows release controls rather than retaining stale input.
    if (!GetActiveFlag()) return {};
    return {
        CheckHitKey(KEY_INPUT_W) != 0,
        CheckHitKey(KEY_INPUT_S) != 0,
        CheckHitKey(KEY_INPUT_A) != 0,
        CheckHitKey(KEY_INPUT_D) != 0,
        CheckHitKey(KEY_INPUT_LSHIFT) != 0 || CheckHitKey(KEY_INPUT_RSHIFT) != 0,
        CheckHitKey(KEY_INPUT_LCONTROL) != 0 || CheckHitKey(KEY_INPUT_RCONTROL) != 0,
        CheckHitKey(KEY_INPUT_R) != 0,
        CheckHitKey(KEY_INPUT_ESCAPE) != 0,
        CheckHitKey(KEY_INPUT_SPACE) != 0,
        CheckHitKey(KEY_INPUT_F) != 0,
        CheckHitKey(KEY_INPUT_TAB) != 0
    };
}

std::vector<EnemyAI> createInitialEnemies() {
    std::vector<EnemyAI> list;
    list.reserve(4);
    list.emplace_back(0, Vec3{300.0, 140.0, 500.0}, Vec3{-0.5, 0.0, -0.8});
    list.emplace_back(1, Vec3{-350.0, 150.0, 450.0}, Vec3{0.5, 0.0, -0.8});
    list.emplace_back(2, Vec3{0.0, 160.0, 700.0}, Vec3{0.0, 0.0, -1.0});
    list.emplace_back(3, Vec3{400.0, 130.0, -200.0}, Vec3{-0.8, 0.0, 0.6});
    return list;
}

} // namespace

int Game::run() {
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText(L"SkyCombat - M5 Combat + AI");
    SetAlwaysRunFlag(TRUE);
    if (DxLib_Init() == -1) return 1;

    SkySystem skySystem;
    int bgR = int(skySystem.colors().horizon.x * 255.0 + 0.5);
    int bgG = int(skySystem.colors().horizon.y * 255.0 + 0.5);
    int bgB = int(skySystem.colors().horizon.z * 255.0 + 0.5);

    SetDrawScreen(DX_SCREEN_BACK);
    SetBackgroundColor(bgR, bgG, bgB);
    SetCameraNearFar(0.1f, 5000.0f);

    TerrainSystem terrainSystem;
    terrainSystem.initialize();

    VegetationSystem vegetationSystem;
    vegetationSystem.build(terrainSystem, terrainSystem.biomes());

    CloudSystem cloudSystem;
    cloudSystem.initialize();

    SpeedEffects speedEffects;
    AudioModel audioModel;
    WeaponSystem weaponSystem;
    TargetingSystem targetingSystem;
    MissionSystem missionSystem(4);

    std::vector<EnemyAI> enemies = createInitialEnemies();

    TerrainRenderer terrainRenderer;
    terrainRenderer.setupEnvironment(skySystem);

    Aircraft current;
    current.reset();
    Aircraft previous = current;

    FlightController controller;
    ChaseCamera camera;
    camera.reset(current, &terrainSystem);

    AircraftRenderer aircraftRenderer;
    Input input;
    Time time;

    double fpsTime = 0, fps = 0;
    int frames = 0;

    while (ProcessMessage() == 0) {
        const auto keys = poll();
        const double dt = time.tick();

        // Edge actions are latched at render cadence so a short R press is not lost.
        auto state = input.update(keys);
        if (state.quit) break;

        if (state.reset) {
            current.reset();
            previous = current;
            camera.reset(current, &terrainSystem);
            time.reset();
            weaponSystem.reset();
            targetingSystem.reset();
            missionSystem.reset(4);
            enemies = createInitialEnemies();
        } else {
            while (time.consumeStep()) {
                previous = current;

                // Player flight update
                if (!current.crashed && current.health > 0.0) {
                    controller.update(current, state.flight, Time::fixedDelta, &terrainSystem);

                    // Player weapon firing
                    if (state.fireGun) {
                        Vec3 muzzle = current.transform.position + current.transform.forward() * 4.0;
                        weaponSystem.firePlayerGun(muzzle, current.transform.forward(), current.speed);
                    }

                    // Targeting cycle and missile launch
                    std::vector<Vec3> enemyPositions;
                    std::vector<bool> enemyAlive;
                    enemyPositions.reserve(enemies.size());
                    enemyAlive.reserve(enemies.size());
                    for (const auto& e : enemies) {
                        enemyPositions.push_back(e.aircraft().transform.position);
                        enemyAlive.push_back(e.isAlive());
                    }

                    if (state.switchTarget) {
                        targetingSystem.cycleTarget(enemyPositions, enemyAlive);
                    }

                    targetingSystem.update(Time::fixedDelta, current.transform.position,
                                           current.transform.forward(), enemyPositions, enemyAlive);

                    if (state.fireMissile) {
                        Vec3 missileSpawn = current.transform.position - current.transform.up() * 1.5;
                        weaponSystem.firePlayerMissile(missileSpawn, current.transform.forward(),
                                                      targetingSystem.selectedTargetIndex(),
                                                      targetingSystem.hasValidLock(),
                                                      current.speed);
                    }
                } else {
                    current.crashed = true;
                }

                // Update enemies AI
                for (size_t i = 0; i < enemies.size(); ++i) {
                    auto& e = enemies[i];
                    if (!e.isAlive()) continue;

                    bool locked = (targetingSystem.selectedTargetIndex() == int(i) &&
                                   targetingSystem.hasValidLock());

                    bool missileIncoming = false;
                    for (const auto& m : weaponSystem.missiles()) {
                        if (m.isActive() && m.targetId() == int(i)) {
                            missileIncoming = true;
                            break;
                        }
                    }

                    e.update(Time::fixedDelta, current, locked, missileIncoming,
                             &terrainSystem, &weaponSystem);
                }

                // Update combat simulation: bullets, missiles, collisions
                std::vector<Aircraft> enemyAircrafts;
                enemyAircrafts.reserve(enemies.size());
                for (const auto& e : enemies) {
                    enemyAircrafts.push_back(e.aircraft());
                }

                weaponSystem.update(Time::fixedDelta, &terrainSystem, current, enemyAircrafts);

                // Write back damage to enemy AI instances
                for (size_t i = 0; i < enemies.size(); ++i) {
                    enemies[i].aircraft().health = enemyAircrafts[i].health;
                    enemies[i].aircraft().crashed = enemyAircrafts[i].crashed;
                }

                // Update mission status
                missionSystem.update(Time::fixedDelta, current, enemies);
            }
        }

        const auto rendered = interpolate(previous, current, time.alpha());
        camera.update(rendered, dt, &terrainSystem);
        const auto& pose = camera.pose();

        // Update cloud drift and chunk LODs
        cloudSystem.update(dt);
        terrainSystem.updateLODs(pose.position);

        ClearDrawScreen();
        SetUseZBuffer3D(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetUseBackCulling(FALSE);
        SetUseLighting(FALSE);

        SetCameraPositionAndTargetAndUpVec(vector(pose.position), vector(pose.target), vector(pose.up));
        SetupCamera_Perspective(float(radians(pose.fovDegrees)));

        // Render 3D scene
        terrainRenderer.drawChunks(terrainSystem.chunks());
        terrainRenderer.drawDistantMountains(terrainSystem.distantMountains());
        terrainRenderer.drawVegetation(vegetationSystem);
        terrainRenderer.drawClouds(cloudSystem);

        // Render player aircraft
        if (!rendered.crashed && rendered.health > 0.0) {
            aircraftRenderer.draw(rendered);
        }

        // Render living enemies
        for (size_t i = 0; i < enemies.size(); ++i) {
            const auto& e = enemies[i];
            if (!e.isAlive()) continue;
            bool isTgt = (targetingSystem.selectedTargetIndex() == int(i));
            bool isLck = (isTgt && targetingSystem.hasValidLock());
            aircraftRenderer.drawEnemy(e.aircraft(), isTgt, isLck);
        }

        // Render bullets and missiles
        for (const auto& b : weaponSystem.bullets()) {
            if (b.active) aircraftRenderer.drawBullet(b);
        }
        for (const auto& m : weaponSystem.missiles()) {
            if (m.isActive()) aircraftRenderer.drawMissile(m);
        }

        fpsTime += dt;
        ++frames;
        if (fpsTime >= 0.5) {
            fps = frames / fpsTime;
            frames = 0;
            fpsTime = 0;
        }

        SetUseZBuffer3D(FALSE);
        SetWriteZBuffer3D(FALSE);

        // 2D Speed lines effect
        double speedIntensity = speedEffects.evaluateIntensity(rendered.speed, state.flight.boost);
        speedEffects.draw(speedIntensity);

        // Audio update (applies parameters only if valid sound handles exist; no-op if -1)
        auto audioParams = audioModel.evaluate(rendered.speed, state.flight.throttle, state.flight.boost);
        audioModel.updateDXLib(-1, -1, audioParams);

        // Radar data and rendering
        auto radarData = Radar::createData(rendered, enemies,
                                           targetingSystem.selectedTargetIndex(),
                                           targetingSystem.hasValidLock());
        Radar::draw(radarData);

        // HUD data model and string drawing
        auto biome = terrainSystem.biomes().classifyBiome(rendered.transform.position.x,
                                                          rendered.transform.position.z,
                                                          rendered.transform.position.y,
                                                          {0, 1, 0});
        auto biomeName = BiomeSystem::biomeName(biome);

        double targetDist = 0.0;
        int tgtIdx = targetingSystem.selectedTargetIndex();
        if (tgtIdx >= 0 && tgtIdx < int(enemies.size()) && enemies[tgtIdx].isAlive()) {
            targetDist = length(enemies[tgtIdx].aircraft().transform.position - rendered.transform.position);
        }

        auto hudData = HUD::createData(rendered, fps, biomeName,
                                       missionSystem.enemiesRemaining(),
                                       tgtIdx, targetDist,
                                       targetingSystem.lockState(),
                                       missionSystem.status());
        HUD::draw(hudData);

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

} // namespace sky
