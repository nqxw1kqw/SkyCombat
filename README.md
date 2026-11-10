# SkyCombat — M2 Terrain + Terrain-aware Flight

C++20 / DXLib, Windows x64. M2 implements the procedural 2 km × 2 km heightfield,
triangle-consistent collision surface, low-poly terrain mesh generation and rendering,
terrain-aware flight assist and collision states, world boundary handling, and chase
camera ground avoidance.

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
these commands are not a guarantee for every SDK release. M2 uses code-generated
analytic terrain and aircraft triangle meshes with no external model or texture dependencies.

Run portable tests on Linux:

```bash
./tests/run_portable.sh
```

The script builds only pure logic with `g++ -std=c++20`, warnings as errors, in a
temporary directory. Total test count: **63 passed, 0 failed**.
- MathTests: **8 passed, 0 failed**.
- FlightTests: **24 passed, 0 failed**.
- TerrainTests: **31 passed, 0 failed**.

Tests verify rotation axes, quaternion interpolation, long rotations, input mapping,
fixed-step timing at 30/60/144 FPS, speed bounds, bank decay, reset, camera smoothing/FOV/loops,
heightfield determinism, GroundHeight consistency against the generated mesh across
hundreds of sample points, world bounds behaviour, clearance/collision cases, and
terrain assist trigger ranges.

## Terrain & GroundHeight Architecture

Terrain elevation follows the spec section 6 procedural heightfield formula:

$$H(x, z) = \mathrm{Hills}(x, z) + \mathrm{MountainMask}(x, z) \cdot \mathrm{Mountains}(x, z) + \mathrm{Detail}(x, z) - \mathrm{ValleyMask}(x, z) \cdot \mathrm{ValleyDepth}(x, z)$$

with a fixed seed (`1337`) producing deterministic heights across platforms:
- **Center (0, 0):** Flat grassland airfield; spawn at $(0, 120, 0)$ provides $>100\,\text{m}$ clearance.
- **North ($+Z > 100\,\text{m}$):** High mountains and elevated ridges rising to $150\text{--}200\,\text{m}$.
- **South ($-Z < -50\,\text{m}$):** Winding canyon corridor following $x(z) = 180\sin(0.006z) + 70\cos(0.014z)$, carved $20\text{--}30\,\text{m}$ deep.
- **East & West:** Rolling meadows.

`IHeightProvider::groundHeight(x, z)` uses **triangle-consistent plane interpolation**
matching the exact triangulation of the rendered low-poly mesh (12,800 triangles).
Each grid cell quad is split into two triangles along the $(0,0) \to (1,1)$ diagonal.
Collision queries evaluate the exact plane of the triangle covering $(x, z)$, guaranteeing
zero discrepancy between the visible mesh surface and collision queries.

## Terrain-Aware Flight Rules

1. **Clearance $> 25\,\text{m}$:** Normal assisted arcade flight without intervention.
2. **Clearance $8\text{--}25\,\text{m}$ while descending:** A limited pitch-up assist
   proportional to descent rate and clearance is applied. Does not force automatic
   terrain following or override pilot authority.
3. **Clearance $< 8\,\text{m}$:** Low clearance warning state active (`terrainWarning = true`).
4. **Ground collision ($\text{clearance} \le 0$):** Aircraft enters crash state (`crashed = true`),
   speed is zeroed, and altitude is clamped to `groundHeight` (the aircraft can never sink below the mesh).
   Pressing `R` restores flight to the spawn point.
5. **World boundaries:** Soft-turn guidance steers the aircraft back toward center within $100\,\text{m}$
   of borders ($|x|, |z| > 900\,\text{m}$). Hard clamping keeps the aircraft within the 2 km × 2 km boundary.
6. **Chase camera avoidance:** Camera altitude is clamped to at least $\text{groundHeight} + 2.5\,\text{m}$.

See [world design](docs/WORLD_DESIGN.md), [architecture](docs/ARCHITECTURE.md), and [flight tuning](docs/FLIGHT_TUNING.md).

Controls: W/S pitch up/down; A/D turn left/right with assisted banking; Shift/Ctrl
increase/decrease speed; R reset / respawn; Esc quit.

Windows acceptance still to verify: Debug build/link against installed DXLib SDK,
window size, Esc, focus loss/recovery, controls and reset, nose/turn/bank directions,
camera stability and terrain avoidance, and visual 60 FPS behavior.

Roadmap: next is **M3**, World Rendering (sky dome / gradient, directional lighting, distance fog,
biome palette refinement, and chunking/LOD preparation).
