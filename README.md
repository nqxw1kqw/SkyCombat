# SkyCombat — M1 Flight Prototype

C++20 / DXLib, Windows x64. M1 implements assisted arcade flight, a code-built
low-poly aircraft, a reference ground grid, and a smooth chase camera. No terrain,
collision, world bounds, weapons or AI are implemented. The grid at Y=0 is only an
altitude reference: flying through it is possible in this milestone.

**Windows/DXLib build and interactive behavior are unverified.** Development and
portable tests ran on Linux without Windows or the DXLib SDK. Official DXLib
[3D primitive documentation](https://dxlib.xsrv.jp/function/dxfunc_3d_draw.html) and
[camera documentation](https://dxlib.xsrv.jp/function/dxfunc_3d_camera.html) were
consulted; this does not substitute for compilation against your installed SDK.

Install Visual Studio 2022 Community with Desktop development with C++, Windows
SDK, CMake, and the DXLib Visual C++ x64 SDK. Use the complete SDK directory
containing DxLib.h and its bundled .lib files. Exact commands from spec section 9:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDXLIB_ROOT="C:/SDK/DxLib/プロジェクトに追加すべきファイル_VC用"
cmake --build build --config Debug
.\build\Debug\SkyCombat.exe
```

Adjust the example path to your SDK. `DXLIB_ROOT` may also be an environment
variable. CMake locates DxLib.h and DxLib_x64.lib, adds the SDK library directory,
and relies on the official MSVC header's autolink pragmas to select Debug/Release
libraries and codec dependencies. Keep all supplied libraries together. The target
uses static MSVC runtime (/MTd in Debug, /MT in Release); confirm that the installed
SDK variants match. SDK packaging and dependencies must be verified on Windows;
these commands are not a guarantee for every SDK release. M1 loads no external
assets and requires no asset copy or runtime model loader.

Run portable tests on Linux:

```bash
./tests/run_portable.sh
```

The script builds only pure logic with `g++ -std=c++20`, warnings as errors, in a
temporary directory. MathTests: **8 passed, 0 failed**. FlightTests: **24 passed,
0 failed**. Tests cover rotation axes, quaternion interpolation, long rotations,
input mapping, fixed-step timing at 30/60/144 FPS, speed bounds, bank decay, reset,
and camera smoothing/FOV/loops. Simulated cadence tests do not verify interactive
Windows rendering or input/focus behavior.

Controls: W/S pitch up/down; A/D turn left/right with assisted banking;
Shift/Ctrl increase/decrease speed; R reset; Esc quit. Opposing controls cancel.
Throttle release holds speed. Shift's boost flag is mapped for future use; M1
uses its throttle axis and the same 140 m/s maximum. Debug overlay shows m/s,
altitude in metres, FPS, and bank assist status.

The window is 1280×720 with back-buffer rendering. Message polling continues
while inactive and inactive keyboard state is cleared. Simulation runs at 60 Hz,
frame delta is capped at 0.25 s, and transforms interpolate for rendering. R snaps
both interpolation endpoints and camera to spawn. See
[architecture](docs/ARCHITECTURE.md) and [flight tuning](docs/FLIGHT_TUNING.md).

Windows acceptance still to verify: Debug build/link, window size, Esc, focus
loss/recovery, controls and reset, nose/turn/bank directions, camera stability,
and visual behavior at 30/60/144 FPS. No Windows build or gameplay run is claimed.

Roadmap: next is **M2**, procedural terrain with shared height queries, collision,
clearance assistance, world bounds and basic camera obstruction. The supplied
`assets/models/fighter_jet.obj` remains parked for a later milestone; M1 deliberately
uses its own triangle mesh. World art/palette, combat and AI remain later work.
