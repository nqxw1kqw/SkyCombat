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
#include "ui/FlightReticle.h"
#include "ui/HUD.h"
#include "ui/Radar.h"
#include <DxLib.h>
#include <cmath>
#include <vector>

namespace sky {

namespace {

VECTOR vector(Vec3 v) { return VGet(float(v.x), float(v.y), float(v.z)); }

struct PolledInput {
    KeyboardState keys;
    MouseState mouse;
};

PolledInput poll(bool captureMouse) {
    // Inactive windows release controls rather than retaining stale input.
    static bool mouseArmed = false;
    if (!GetActiveFlag()) { mouseArmed = false; return {}; }
    PolledInput out;
    out.keys = {
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
        CheckHitKey(KEY_INPUT_TAB) != 0,
        CheckHitKey(KEY_INPUT_C) != 0
    };
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    const int buttons = GetMouseInput();
    // Direct Mouse Flight: while gameplay capture is on, the cursor is
    // hidden and re-centred every frame; the mouse input is the movement
    // away from the centre since last frame. The first frame after
    // capture (re)starts discards its delta, so neither the re-centre
    // itself nor an Alt+Tab jump can jerk the aircraft.
    int deltaX = 0, deltaY = 0;
    if (captureMouse) {
        if (mouseArmed) { deltaX = mx - 640; deltaY = my - 360; }
        SetMousePoint(640, 360);
        mouseArmed = true;
    } else {
        mouseArmed = false;
    }
    out.mouse = {deltaX, deltaY,
                 (buttons & MOUSE_INPUT_LEFT) != 0,
                 (buttons & MOUSE_INPUT_RIGHT) != 0,
                 captureMouse};
    return out;
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
    SetMouseDispFlag(FALSE); // cursor hidden in play: captured + re-centred per frame

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
    bool paused = false;
    int cameraPreset = 0; // 0 = chase, 1 = close chase, 2 = nose
    double carryMouseYaw = 0, carryMousePitch = 0;

    while (ProcessMessage() == 0) {
        const auto polled = poll(!paused);
        const double dt = time.tick();

        // Edge actions are latched at render cadence so a short R press is not lost.
        auto state = input.update(polled.keys, polled.mouse, dt);
        if (state.pausePressed) {
            paused = !paused;
            SetMouseDispFlag(paused ? TRUE : FALSE); // pause hands the cursor back
        }
        if (state.switchCamera) {
            cameraPreset = (cameraPreset + 1) % 3;
            camera.setFollowScale(cameraPreset == 1 ? 0.5 : 1.0);
        }

        if (paused) {
            time.reset(); // drain the accumulator so resume does not fast-forward
        } else if (state.reset) {
            carryMouseYaw = 0;
            carryMousePitch = 0;
            current.reset();
            previous = current;
            camera.reset(current, &terrainSystem);
            time.reset();
            weaponSystem.reset();
            targetingSystem.reset();
            missionSystem.reset(4);
            enemies = createInitialEnemies();
        } else {
            // Mouse rotation is per render frame, physics runs on fixed
            // steps: bank the frame's rotation and let the first step of
            // the frame spend it all (frames with no step keep it banked).
            carryMouseYaw += state.flight.mouseYaw;
            carryMousePitch += state.flight.mousePitch;
            state.flight.mouseYaw = carryMouseYaw;
            state.flight.mousePitch = carryMousePitch;
            bool switchTargetOnce = state.switchTarget;
            bool fireMissileOnce = state.fireMissile;
            while (time.consumeStep()) {
                previous = current;

                // Player flight update
                if (!current.crashed && current.health > 0.0) {
                    controller.update(current, state.flight, Time::fixedDelta, &terrainSystem);
                    if (carryMouseYaw != 0.0 || carryMousePitch != 0.0) {
                        state.flight.mouseYaw = 0.0;
                        state.flight.mousePitch = 0.0;
                        carryMouseYaw = 0.0;
                        carryMousePitch = 0.0;
                    }

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

                    // Lock-on is a held action: hold the right mouse button
                    // to acquire; releasing it drops the lock entirely.
                    if (state.lockOn) {
                        if (switchTargetOnce) {
                            targetingSystem.cycleTarget(enemyPositions, enemyAlive);
                            switchTargetOnce = false;
                        }
                        targetingSystem.update(Time::fixedDelta, current.transform.position,
                                               current.transform.forward(), enemyPositions, enemyAlive);
                    } else {
                        targetingSystem.reset();
                    }

                    if (fireMissileOnce) {
                        fireMissileOnce = false;
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

        // Nose camera preset (C cycles chase -> close chase -> nose): sit
        // on the canopy looking down the nose; the chase pose still
        // drives LOD selection.
        CameraPose renderPose = pose;
        if (cameraPreset == 2) {
            renderPose.position = rendered.transform.position + rendered.transform.up() * 1.6 +
                                  rendered.transform.forward() * 8.0;
            renderPose.target = renderPose.position + rendered.transform.forward() * 120.0;
            renderPose.up = rendered.transform.up();
        }

        // Update cloud drift and chunk LODs
        if (!paused) cloudSystem.update(dt);
        terrainSystem.updateLODs(pose.position);

        ClearDrawScreen();
        SetUseZBuffer3D(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetUseBackCulling(FALSE);
        SetUseLighting(FALSE);

        SetCameraPositionAndTargetAndUpVec(vector(renderPose.position), vector(renderPose.target), vector(renderPose.up));
        SetupCamera_Perspective(float(radians(pose.fovDegrees)));

        // Render 3D scene
        terrainRenderer.drawChunks(terrainSystem.chunks());
        terrainRenderer.drawDistantMountains(terrainSystem.distantMountains());
        terrainRenderer.drawVegetation(vegetationSystem);
        terrainRenderer.drawClouds(cloudSystem);

        // Render player aircraft (hidden in nose-camera view)
        if (cameraPreset != 2 && !rendered.crashed && rendered.health > 0.0) {
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

        // Aim reticle (fixed centre cross; relative mouse has no cursor
        // position to mark) + pause overlay
        if (!paused) {
            FlightReticle::draw(640, 360);
        } else {
            DrawString(552, 344, L"PAUSED", GetColor(255, 255, 255));
            DrawString(496, 368, L"Esc: Resume   |   Close window: Quit", GetColor(200, 215, 230));
        }

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

} // namespace sky
