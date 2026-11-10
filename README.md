# SkyCombat — M4 Environment Art + Flight Polish

C++20 / DXLib, Windows x64. M4 implements environment art and flight polish:
VegetationSystem (deterministic batched crossed-quad grass clumps on grassland and
flower tufts strictly inside purple/pink flower biomes, surface clamping, and strict
exclusions on rock/steep slopes/valley floor path), CloudSystem (deterministic low-poly
cloud clusters in [230, 360] m altitude band with slow drift and zero per-frame
allocations), Chase camera polish (positional turn lag, camera roll capped at 12°,
speed-mapped 65–80° FOV, and exponential smoothing), SpeedEffects (speed-line intensity
ramping above 105 m/s threshold with boost bonus and config toggle), AudioModel (pure
throttle/speed mapping for engine and wind audio), and HUD (pure data model with km/h
conversion, altitude, clearance, and assist/warning states).

**Windows/DXLib build, real visuals, and audio playback are unverified.** Development
and portable tests ran on Linux without Windows or the DXLib SDK. Official DXLib
[3D primitive documentation](https://dxlib.xsrv.jp/function/dxfunc_3d_draw.html) and
[sound documentation](https://dxlib.xsrv.jp/function/dxfunc_sound.html) were consulted;
this does not substitute for compilation against your installed SDK.
**Audio asset notice:** No audio waveform files ship with the repository; the DXLib
audio layer applies parameters only if a valid sound handle exists (otherwise it is
a safe no-op).

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
these commands are not a guarantee for every SDK release.

Run portable tests on Linux:

```bash
./tests/run_portable.sh
```

The script builds only pure logic with `g++ -std=c++20`, warnings as errors, in a
temporary directory. Total test count: **147 passed, 0 failed**.
- MathTests: **8 passed, 0 failed**.
- FlightTests: **24 passed, 0 failed**.
- TerrainTests: **31 passed, 0 failed**.
- WorldRenderTests: **33 passed, 0 failed**.
- PolishTests: **51 passed, 0 failed**.

Tests verify rotation axes, quaternion interpolation, long rotations, input mapping,
fixed-step timing at 30/60/144 FPS, speed bounds, bank decay, reset, camera smoothing/FOV/loops,
heightfield determinism, GroundHeight consistency against the generated mesh across
hundreds of sample points, world bounds behaviour, clearance/collision cases,
terrain assist trigger ranges, sky gradient and distance fog evaluation, biome classification
sanity, smooth boundary blending continuity, complete 16-chunk world coverage, chunk edge height
parity at equal LOD, LOD selection with hysteresis stability, distant mountain geometry,
vegetation determinism and batch caps, flower tuft biome constraints (purple flower tufts ONLY
in West, pink flower tufts ONLY in East, on terrain surface via GroundHeight), vegetation terrain
exclusions (none on rock, steep slopes, or valley floor canyon path), cloud determinism and
altitude band [230, 360] m, cloud slow drift and seamless wrapping, camera positional turn lag,
camera roll fraction and 12° hard cap, camera FOV mapping [65, 80]°, speed-line threshold (105 m/s)
and config toggle, monotonic audio parameter mapping with extreme input clamping, and HUD km/h
speed conversion and clearance warning states.

## Environment Art & Flight Polish Architecture (M4)

1. **VegetationSystem (`src/world/VegetationSystem.h/.cpp`):**
   - Deterministic fixed-seed scatter (`seed = 1337`) of low-poly crossed quads (8 vertices, 4 triangles per clump).
   - Merged batches grouped by type (`grassBatch`, `purpleFlowersBatch`, `pinkFlowersBatch`), eliminating per-blade draw calls.
   - Flower tufts grow ONLY inside matching biomes:
     - Purple flowers: Western lowlands ($X < 0\,\text{m}$, `classifyBiome == PurpleFlowerField`, cap 400).
     - Pink flowers: Eastern lowlands ($X > 0\,\text{m}$, `classifyBiome == PinkFlowerField`, cap 400).
     - Grass clumps: Central airfield and upland meadows (Grassland/Highland, cap 1,200).
   - Strict exclusions: zero vegetation on steep slopes ($N_y < 0.85$), mountain rock ($Y > 85\,\text{m}$ or $w_{\text{rock}} > 0.05$), or valley floor canyon path ($w_{\text{valley}} > 0.05$ or $\text{valleyMask} > 0.05$).

2. **CloudSystem (`src/world/CloudSystem.h/.cpp`):**
   - 18 deterministic low-poly cloud clusters formed by overlapping flattened boxes with shaded undersides (`#c4d2e2`) and puffy white tops (`#f8fbff`).
   - Stratified altitude band strictly within $[230, 360]\,\text{m}$.
   - Drifts eastward at $2.5\,\text{m/s}$ with seamless wrap-around at $X = 1200\,\text{m}$.
   - **Zero per-frame allocation:** Updates vertex positions in-place using precomputed cluster vertex spans.

3. **Chase Camera Polish (`src/camera/ChaseCamera.cpp`):**
   - **Positional turn lag:** Displaces camera laterally opposite bank during turns (`lateralLag = right * (-visualBank * 1.8)`).
   - **Camera roll:** Limited fraction of aircraft bank ($20\%$) with a hard cap at $12.0^\circ$ ($0.2094\,\text{rad}$) to maintain horizon readability.
   - **FOV mapping:** Smoothly mapped from $65^\circ$ at minimum speed ($25\,\text{m/s}$) to $80^\circ$ at maximum speed ($140\,\text{m/s}$).
   - **Exponential smoothing:** Frame-rate independent response rates ($6.0\,\text{s}^{-1}$ position, $8.0\,\text{s}^{-1}$ target, $4.0\,\text{s}^{-1}$ FOV).
   - **Terrain avoidance:** Retains M2 margin of $2.5\,\text{m}$ above local ground height.

4. **SpeedEffects (`src/effects/SpeedEffects.h/.cpp`):**
   - Pure intensity evaluator: strictly zero below threshold ($105\,\text{m/s}$ = $378\,\text{km/h}$), ramping linearly to $1.0$ at max speed ($140\,\text{m/s}$).
   - Boost bonus ($+0.20$ intensity) provides immediate visual feedback.
   - Configurable toggle (`enabled`) completely suppresses speed lines when disabled.
   - DXLib layer draws lightweight 2D radial lines; no expensive post-processing shaders.

5. **AudioModel (`src/audio/AudioModel.h/.cpp`):**
   - Pure monotonic mapping of airspeed and throttle to engine volume ($0.35\text{--}1.00$), engine pitch multiplier ($0.80\text{--}1.60$, up to $1.80$ boost), and wind volume ($0.00\text{--}1.00$).
   - Clamped safely against extreme input ranges.
   - DXLib sound layer applies parameters only if valid sound handles exist.

6. **HUD Data Model (`src/ui/HUD.h/.cpp`):**
   - Pure `HUDData` struct decouples formatting from DXLib: computes speed in $\text{km/h}$ ($\text{speed}_{\text{m/s}} \times 3.6$), altitude in metres, clearance, assist/warning state (`Normal`, `AssistEngaged`, `LowClearanceWarning`, `Crashed`), and FPS.
   - DXLib layer only draws the formatted strings.

See [world design](docs/WORLD_DESIGN.md), [architecture](docs/ARCHITECTURE.md), and [flight tuning](docs/FLIGHT_TUNING.md).

Controls: W/S pitch up/down; A/D turn left/right with assisted banking; Shift/Ctrl
increase/decrease speed; R reset / respawn; Esc quit.

Windows acceptance still to verify: Debug build/link against installed DXLib SDK,
window size, Esc, focus loss/recovery, controls and reset, nose/turn/bank directions,
camera stability and terrain avoidance, visual 60 FPS behavior, and audio playback.

Roadmap: next is **M5**, Combat + AI (machine guns, lock-on missiles, enemy AI FSM, radar).
