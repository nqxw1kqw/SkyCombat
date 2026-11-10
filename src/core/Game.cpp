#include "Game.h"
#include "Time.h"
#include "Input.h"
#include "aircraft/FlightController.h"
#include "camera/ChaseCamera.h"
#include "rendering/AircraftRenderer.h"
#include "rendering/TerrainRenderer.h"
#include "world/TerrainSystem.h"
#include "world/SkySystem.h"
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
    SetMainWindowText(L"SkyCombat - M3 World Rendering");
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

        // Update chunk LODs based on current camera view position
        terrainSystem.updateLODs(pose.position);

        ClearDrawScreen();
        SetUseZBuffer3D(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetUseBackCulling(FALSE);
        SetUseLighting(FALSE);

        SetCameraPositionAndTargetAndUpVec(vector(pose.position), vector(pose.target), vector(pose.up));
        SetupCamera_Perspective(float(radians(pose.fovDegrees)));

        // Render terrain chunks at selected LODs, distant mountain proxy, and aircraft
        terrainRenderer.drawChunks(terrainSystem.chunks());
        terrainRenderer.drawDistantMountains(terrainSystem.distantMountains());
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

        auto biome = terrainSystem.biomes().classifyBiome(rendered.transform.position.x,
                                                          rendered.transform.position.z,
                                                          rendered.transform.position.y,
                                                          {0, 1, 0});
        auto biomeName = BiomeSystem::biomeName(biome);

        DrawFormatString(16, 16, GetColor(255, 255, 255),
                         L"M3 | %.1f m/s | altitude %.1f m | clearance %.1f m | %.1f FPS",
                         rendered.speed, rendered.transform.position.y, rendered.clearance, fps);
        DrawFormatString(16, 40, GetColor(240, 240, 255),
                         L"Biome: %S | 16 Chunks (500x500m) with 3-LOD distance selection",
                         biomeName.data());
        DrawString(16, 64, L"W/S pitch | A/D turn | Shift/Ctrl throttle | R reset | Esc quit",
                   GetColor(255, 255, 255));

        if (rendered.crashed) {
            DrawString(16, 88, L"[CRASHED - PRESS R TO RESPAWN]", GetColor(255, 80, 80));
        } else if (rendered.terrainWarning) {
            DrawString(16, 88, L"[WARNING: LOW CLEARANCE (<8m)]", GetColor(255, 60, 60));
        } else if (rendered.terrainAssist) {
            DrawString(16, 88, L"[TERRAIN ASSIST ACTIVE (8-25m)]", GetColor(255, 230, 80));
        } else {
            DrawString(16, 88, L"[CLEARANCE NORMAL (>25m)]", GetColor(160, 240, 160));
        }

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}

} // namespace sky
