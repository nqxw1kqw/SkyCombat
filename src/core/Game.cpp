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
#include "ui/HUD.h"
#include <DxLib.h>
#include <cmath>

namespace sky {

namespace {

VECTOR vector(Vec3 v) { return VGet(float(v.x), float(v.y), float(v.z)); }

KeyboardState poll() {
    // Inactive windows release controls rather than retaining stale input.
    if (!GetActiveFlag()) return {};
    return {CheckHitKey(KEY_INPUT_W) != 0, CheckHitKey(KEY_INPUT_S) != 0,
            CheckHitKey(KEY_INPUT_A) != 0, CheckHitKey(KEY_INPUT_D) != 0,
            CheckHitKey(KEY_INPUT_LSHIFT) != 0 || CheckHitKey(KEY_INPUT_RSHIFT) != 0,
            CheckHitKey(KEY_INPUT_LCONTROL) != 0 || CheckHitKey(KEY_INPUT_RCONTROL) != 0,
            CheckHitKey(KEY_INPUT_R) != 0, CheckHitKey(KEY_INPUT_ESCAPE) != 0};
}

} // namespace

int Game::run() {
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText(L"SkyCombat - M4 Environment Art + Flight Polish");
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
        } else {
            while (time.consumeStep()) {
                previous = current;
                controller.update(current, state.flight, Time::fixedDelta, &terrainSystem);
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

        // Render terrain chunks, distant mountains, vegetation batches, clouds, and aircraft
        terrainRenderer.drawChunks(terrainSystem.chunks());
        terrainRenderer.drawDistantMountains(terrainSystem.distantMountains());
        terrainRenderer.drawVegetation(vegetationSystem);
        terrainRenderer.drawClouds(cloudSystem);
        aircraftRenderer.draw(rendered);

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

        // HUD data model and string drawing
        auto biome = terrainSystem.biomes().classifyBiome(rendered.transform.position.x,
                                                          rendered.transform.position.z,
                                                          rendered.transform.position.y,
                                                          {0, 1, 0});
        auto biomeName = BiomeSystem::biomeName(biome);
        auto hudData = HUD::createData(rendered, fps, biomeName);
        HUD::draw(hudData);

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

} // namespace sky
