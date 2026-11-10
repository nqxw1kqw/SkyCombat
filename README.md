# SkyCombat — M3 World Rendering

C++20 / DXLib, Windows x64. M3 implements the atmospheric world rendering pipeline:
SkySystem (sky dome gradient, directional sun lighting, and distance fog), BiomeSystem
(six biomes with smooth $C^1$ boundary blending matching the palette reference),
terrain chunking ($4\times 4 = 16$ chunks of $500\text{ m}\times 500\text{ m}$), 3-level
distance-based LOD selection with hysteresis, chunk border height parity, and distant
northern mountain silhouettes.

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
these commands are not a guarantee for every SDK release.

Run portable tests on Linux:

```bash
./tests/run_portable.sh
```

The script builds only pure logic with `g++ -std=c++20`, warnings as errors, in a
temporary directory. Total test count: **96 passed, 0 failed**.
- MathTests: **8 passed, 0 failed**.
- FlightTests: **24 passed, 0 failed**.
- TerrainTests: **31 passed, 0 failed**.
- WorldRenderTests: **33 passed, 0 failed**.

Tests verify rotation axes, quaternion interpolation, long rotations, input mapping,
fixed-step timing at 30/60/144 FPS, speed bounds, bank decay, reset, camera smoothing/FOV/loops,
heightfield determinism, GroundHeight consistency against the generated mesh across
hundreds of sample points, world bounds behaviour, clearance/collision cases,
terrain assist trigger ranges, sky gradient and distance fog evaluation, biome classification
sanity, smooth boundary blending continuity, complete 16-chunk world coverage, chunk edge height
parity at equal LOD, LOD selection with hysteresis stability, and distant mountain geometry.

## World Rendering Architecture (M3)

1. **SkySystem (`src/world/SkySystem.h/.cpp`):**
   - Pure data container for zenith color (`#4794e6`), horizon haze (`#b2d6f2`), sun vector $(0.4, 0.8, 0.3)$, and sun color.
   - Distance fog starting at $500\,\text{m}$ and reaching full density at $3500\,\text{m}$, seamlessly blending distant terrain into the horizon.

2. **BiomeSystem (`src/world/BiomeSystem.h/.cpp`):**
   - Classifies terrain into six distinct biomes based on position, altitude, and slope:
     - **Grassland:** Central airfield and open dogfight space (`#8cc473`).
     - **PurpleFlowerField:** Western lowlands ($X < -120\,\text{m}$) (`#bc94e2`).
     - **PinkFlowerField:** Eastern lowlands ($X > 120\,\text{m}$) (`#edb5c9`).
     - **Highland:** Rolling upland meadows ($Y \in [35, 85\,\text{m}]$) (`#94b07a`).
     - **MountainRock:** Northern peaks and steep cliffs (`#949c8f`, blending to snow `#d1d9e0` at peaks $>160\,\text{m}$).
     - **ValleyFloor:** Southern winding canyon bed (`#c7ad80`).
   - Pure normalized smoothstep blending eliminates tile seams and hard color edges.

3. **Terrain Chunking & 3-Level LOD (`src/world/TerrainChunk.h/.cpp`):**
   - The 2 km × 2 km world is divided into $4 \times 4 = 16$ chunks ($500\,\text{m} \times 500\,\text{m}$ each).
   - Each chunk generates 3 mesh levels:
     - **LOD 0 (High):** $25\,\text{m}$ cells, 800 triangles/chunk (12,800 world total, exact M2 parity).
     - **LOD 1 (Medium):** $50\,\text{m}$ cells, 200 triangles/chunk.
     - **LOD 2 (Low):** $100\,\text{m}$ cells, 50 triangles/chunk.
   - Distance-based LOD selection with $100\,\text{m}$ hysteresis margin ($650\,\text{m} / 550\,\text{m}$ and $1200\,\text{m} / 1100\,\text{m}$) eliminates popping.
   - Shared border vertices between adjacent chunks at equal LOD have identical heights down to $< 10^{-10}\,\text{m}$, preventing cracks.

4. **Distant Mountain Silhouettes:**
   - Lightweight proxy ridge beyond the northern boundary ($Z \in [1050, 1700]\,\text{m}$) with 8 peaks rising to $180\text{--}350\,\text{m}$ altitude, providing visual horizon depth without terrain streaming.

5. **Terrain-Aware Flight & Collision Parity:**
   - `GroundHeight(x, z)` query evaluates the exact triangle-consistent plane equation of the full-resolution mesh, ensuring 100% parity with M2 collision and flight assist rules.

See [world design](docs/WORLD_DESIGN.md), [architecture](docs/ARCHITECTURE.md), and [flight tuning](docs/FLIGHT_TUNING.md).

Controls: W/S pitch up/down; A/D turn left/right with assisted banking; Shift/Ctrl
increase/decrease speed; R reset / respawn; Esc quit.

Windows acceptance still to verify: Debug build/link against installed DXLib SDK,
window size, Esc, focus loss/recovery, controls and reset, nose/turn/bank directions,
camera stability and terrain avoidance, and visual 60 FPS behavior.

Roadmap: next is **M4**, Environment Art + Flight Polish (vegetation batching/billboards, low-poly clouds, camera lag polish, and speed line cues).
