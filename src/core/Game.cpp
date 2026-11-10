#include "Game.h"
#include "Time.h"
#include "Input.h"
#include "aircraft/FlightController.h"
#include "camera/ChaseCamera.h"
#include "rendering/AircraftRenderer.h"
#include <DxLib.h>
#include <cmath>
namespace sky {
namespace {
VECTOR vector(Vec3 v) { return VGet(float(v.x),float(v.y),float(v.z)); }
KeyboardState poll() {
    // Inactive windows release controls rather than retaining stale input.
    if(!GetActiveFlag()) return {};
    return {CheckHitKey(KEY_INPUT_W)!=0,CheckHitKey(KEY_INPUT_S)!=0,
        CheckHitKey(KEY_INPUT_A)!=0,CheckHitKey(KEY_INPUT_D)!=0,
        CheckHitKey(KEY_INPUT_LSHIFT)!=0||CheckHitKey(KEY_INPUT_RSHIFT)!=0,
        CheckHitKey(KEY_INPUT_LCONTROL)!=0||CheckHitKey(KEY_INPUT_RCONTROL)!=0,
        CheckHitKey(KEY_INPUT_R)!=0,CheckHitKey(KEY_INPUT_ESCAPE)!=0};
}
void grid(Vec3 position) {
    double x=std::floor(position.x/50)*50,z=std::floor(position.z/50)*50;
    auto color=GetColor(96,133,119);
    for(int i=-20;i<=20;++i) {
        DrawLine3D(vector({x+i*50,0,z-1000}),vector({x+i*50,0,z+1000}),color);
        DrawLine3D(vector({x-1000,0,z+i*50}),vector({x+1000,0,z+i*50}),color);
    }
}
}
int Game::run() {
    ChangeWindowMode(TRUE);
    SetGraphMode(1280,720,32);
    SetMainWindowText(L"SkyCombat - M1 Flight Prototype");
    SetAlwaysRunFlag(TRUE);
    if(DxLib_Init()==-1) return 1;
    SetDrawScreen(DX_SCREEN_BACK);
    SetBackgroundColor(126,183,215);
    SetCameraNearFar(0.1f,4000.0f);
    Aircraft current; current.reset(); Aircraft previous=current;
    FlightController controller; ChaseCamera camera; camera.reset(current);
    AircraftRenderer renderer; Input input; Time time;
    double fpsTime=0,fps=0; int frames=0;
    while(ProcessMessage()==0) {
        const auto keys=poll();
        const double dt=time.tick();
        // Edge actions are latched at render cadence so a short R press is not lost.
        auto state=input.update(keys);
        if(state.quit) break;
        if(state.reset) {
            current.reset(); previous=current; camera.reset(current); time.reset();
        } else {
            while(time.consumeStep()) {
                previous=current;
                controller.update(current,state.flight,Time::fixedDelta);
            }
        }
        const auto rendered=interpolate(previous,current,time.alpha());
        camera.update(rendered,dt);
        const auto& pose=camera.pose();
        ClearDrawScreen();
        SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
        SetUseBackCulling(FALSE); SetUseLighting(FALSE);
        SetCameraPositionAndTargetAndUpVec(vector(pose.position),vector(pose.target),vector(pose.up));
        SetupCamera_Perspective(float(radians(pose.fovDegrees)));
        grid(rendered.transform.position); renderer.draw(rendered);
        fpsTime+=dt; ++frames;
        if(fpsTime>=0.5) { fps=frames/fpsTime; frames=0; fpsTime=0; }
        SetUseZBuffer3D(FALSE); SetWriteZBuffer3D(FALSE);
        DrawFormatString(16,16,GetColor(255,255,255),L"M1 | %.1f m/s | altitude %.1f m | %.1f FPS",rendered.speed,rendered.transform.position.y,fps);
        DrawString(16,40,L"W/S pitch | A/D turn | Shift/Ctrl throttle | R reset | Esc quit",GetColor(255,255,255));
        DrawString(16,64,L"Bank assist ON | Reference grid only - no ground collision (M2)",GetColor(255,255,255));
        ScreenFlip();
    }
    DxLib_End(); return 0;
}
}
