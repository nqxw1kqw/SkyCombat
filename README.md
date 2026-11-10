# SkyCombat — 3D Flight Combat Game (Milestones M1–M5 Complete)

C++20 / DXLib, Windows x64. SkyCombat is a third-person arcade flight action combat game featuring assisted flight dynamics, procedural multithreaded-ready chunked terrain with LODs and biomes, environment art and flight polish, and full 3D combat with homing missiles and reactive enemy AI.

Milestones M1 through M5 are fully implemented. **202 / 202 portable tests are passing**.

---

## Controls

| Control | Action | Details |
| :--- | :--- | :--- |
| **W / S** | Pitch Down / Up | Assists flight attitude; inverted flight enabled through pure quaternion rotation |
| **A / D** | Turn Left / Right | Coordinated yaw turn with assisted visual and aerodynamic banking |
| **LShift / LCtrl** | Throttle Up / Down | Accelerate / decelerate airspeed ($25\text{--}140\,\text{m/s}$) |
| **Space** | Fire Machine Gun | Rapid-fire cannon ($600\,\text{m/s}$, $0.08\,\text{s}$ cooldown, swept anti-tunneling) |
| **F** | Launch Missile | Fires homing missile ($220\,\text{m/s}$, $45^\circ/\text{s}$ turn limit); requires target lock |
| **Tab** | Cycle Target | Selects nearest enemy or cycles to next living target within range |
| **R** | Reset / Restart | Respawns player and resets flight controller, camera, and combat mission |
| **Esc** | Quit | Closes application window |

---

## Build & Run

### Portable Unit Tests (Linux / macOS / Windows bash)

The core physics, math, terrain generation, vegetation scatter, cloud dynamics, audio model, flight assist, weapons, homing missiles, targeting, enemy AI FSM, and radar systems are built with zero DXLib dependencies in pure C++20:

```bash
./tests/run_portable.sh
```

All **202 portable tests** run with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2`:
- **MathTests:** **8 passed, 0 failed** (vector arithmetic, quaternion rotation, slerp, axis-angle).
- **FlightTests:** **24 passed, 0 failed** (speed bounds, bank decay, assisted flight, deterministic fixed steps, reset).
- **TerrainTests:** **31 passed, 0 failed** (heightfield determinism, surface consistency, world boundaries, clearance, chunk coverage).
- **WorldRenderTests:** **33 passed, 0 failed** (sky gradient, distance fog, biome classification, smooth blending, LOD hysteresis, distant mountains).
- **PolishTests:** **51 passed, 0 failed** (vegetation scatter & biome constraints, cloud drift & wrapping, chase camera turn lag/roll cap/FOV mapping, speed-effects threshold, audio model mapping, HUD formatting).
- **CombatTests:** **55 passed, 0 failed** (gun cooldown & pool exhaustion/reuse, swept bullet-to-sphere collision & anti-tunneling, bullet terrain collision, targeting cone/range & Tab cycling, missile turn-rate limit & interception, enemy AI FSM transitions, enemy terrain avoidance, mission clear/fail lifecycle, radar relative projection).

### Windows x64 Build (Visual Studio 2022 + DXLib)

Prerequisites:
- Visual Studio 2022 Community with **Desktop development with C++** and Windows SDK
- CMake 3.20+
- DXLib Visual C++ x64 SDK

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDXLIB_ROOT="C:/SDK/DxLib/プロジェクトに追加すべきファイル_VC用"
cmake --build build --config Release
.\build\Release\SkyCombat.exe
```

Adjust `-DDXLIB_ROOT` to point to your installed DXLib package directory containing `DxLib.h` and the `.lib` files. `DXLIB_ROOT` may also be set as an environment variable.

---

## Milestone Summary & Architecture

### M1 — Pure Flight Core & Assisted Flight Model
- **Math & Kinematics (`src/core/Math.h`, `Transform.h`):** Double-precision 3D vectors and quaternions. Loops and rolls operate free of gimbal lock.
- **Flight Controller (`src/aircraft/`):** Fixed-timestep simulation ($60\,\text{Hz}$) with accumulator cap ($0.25\,\text{s}$). Coordinated turn yawing, auto-recovery bank decay, speed integration ($25\text{--}140\,\text{m/s}$), and physical vs. visual bank separation.
- **Chase Camera (`src/camera/ChaseCamera.h/.cpp`):** Exponential spring-damper following physical orientation through loops.

### M2 — Procedural Terrain & Low-Altitude Flight Assist
- **Terrain System (`src/world/TerrainSystem.h/.cpp`):** Deterministic multi-octave simplex/perlin noise generating central airfield, eastern canyons, western lowlands, and northern mountains.
- **World Collision & Boundaries (`src/world/WorldCollision.h/.cpp`):** Boundary damping nudging aircraft back toward the center; soft low-clearance pitch-up assist preventing inadvertent crashes; crash resolution on ground impact.

### M3 — Chunked Terrain Rendering, LODs & Biomes
- **Chunked LOD Mesh (`src/world/TerrainChunk.h/.cpp`):** $4 \times 4$ chunk grid ($2400 \times 2400\,\text{m}$) with 3 discrete LOD levels ($32 \times 32$, $16 \times 16$, $8 \times 8$), geomorphic stitching along chunk borders, and hysteresis-stabilized LOD switching.
- **Biome Classification & Shading (`src/world/BiomeSystem.h/.cpp`):** 5-layer biome classification (Runway, Grassland, Canyon, Rock, Snow) with continuous triplanar blending and distance-fog integration.

### M4 — Environment Art & Flight Polish
- **VegetationSystem (`src/world/VegetationSystem.h/.cpp`):** Deterministic scatter of low-poly crossed quads. Merged batches grouped by type (`grassBatch`, `purpleFlowersBatch`, `pinkFlowersBatch`). Flower tufts grow strictly inside designated biome sectors (purple in West, pink in East).
- **CloudSystem (`src/world/CloudSystem.h/.cpp`):** 18 deterministic low-poly cloud clusters in the $[230, 360]\,\text{m}$ altitude band with steady eastward drift, seamless wrapping, and zero per-frame heap allocations.
- **Camera & Effect Polish:** Camera positional turn lag, horizon-preserving roll limit ($12^\circ$), speed-mapped FOV ($65^\circ\text{--}80^\circ$); speed lines above $105\,\text{m/s}$; pure audio model mapping speed and throttle to engine/wind volume and pitch.

### M5 — Combat & AI Systems
- **WeaponSystem (`src/combat/WeaponSystem.h/.cpp`):**
  - Machine gun with fixed 120-bullet pool, $0.08\,\text{s}$ fire cooldown, and $600\,\text{m/s}$ projectile speed.
  - **Swept Anti-Tunneling:** Substep segment-to-sphere collision detection tests against aircraft hit-spheres ($r = 3.5\,\text{m}$) so high-speed bullets never pass through targets between frames.
  - Bullet terrain collision via `GroundHeight`.
  - Hit and explosion events streamed to renderer and audio.
- **TargetingSystem (`src/combat/TargetingSystem.h/.cpp`):**
  - Lock-on evaluator with range limit ($1200\,\text{m}$) and view-cone check ($30^\circ$ half-angle).
  - Short acquisition timer ($0.8\,\text{s}$) to transition from `Acquiring` to `Locked`.
  - `Tab` key cycles through living targets in deterministic distance order.
- **Homing Missile (`src/combat/Missile.h/.cpp`):**
  - Launched only with a confirmed lock (`F` key).
  - Rate-limited turn rate ($45^\circ/\text{s} = 0.7854\,\text{rad/s}$) preventing instant snaps and producing sweeping pursuit arcs.
  - Proximity detonation radius ($6.0\,\text{m}$), $5.0\,\text{s}$ fuel/lifetime limit, $100\,\text{HP}$ lethal blast damage.
  - Trail point history buffer for rendering smoke trails.
- **Enemy AI (`src/ai/EnemyAI.h/.cpp`, `SteeringBehavior.h/.cpp`):**
  - 4 enemy fighters utilizing the assisted flight controller with turn-rate constraints.
  - **5-State FSM:**
    - `PATROL`: Cruises between tactical waypoints at cruising speed ($60\,\text{m/s}$).
    - `CHASE`: Engages player once inside $1000\,\text{m}$ detection bubble.
    - `ATTACK`: Fires nose cannon when player is within $350\,\text{m}$ and $25^\circ$ bore angle.
    - `EVADE`: Initiates break turn and evasive weaves when locked by player, targeted by missile, or taking damage.
    - `RETREAT`: Breaks off and flees at full throttle when health drops below $25\%$.
  - **Terrain Clearance Override:** AI queries `GroundHeight` and overrides pitch with an assertive pull-up when altitude drops below $35\,\text{m}$.
  - World boundary soft-turn keeps enemies inside the combat arena.
- **MissionSystem (`src/combat/MissionSystem.h/.cpp`):**
  - Tracks targets remaining; transitions from `Active` to `Cleared` when all enemies are destroyed, or to `Failed` if the player's aircraft is destroyed or crashes.
  - Reset (`R`) restores player, enemies, weapons, and mission state cleanly.
- **Radar & HUD (`src/ui/Radar.h/.cpp`, `HUD.h/.cpp`):**
  - Pure `RadarData` model projects targets relative to player heading into $[-1, 1]$ screen coordinates, displaying range rings, standard blips, and diamond lock-on reticles.
  - HUD displays player HP bar/percentage, remaining targets count, target name, distance, lock progress, and victory/defeat announcement banners.

---

## Honest Status & Verification Disclosure

All gameplay logic, kinematics, combat, AI, and math were engineered and validated via a **100% portable test harness on Linux** with zero platform-specific dependencies.

### What Has Been Formally Verified
1. **Kinematics & Control:** Fixed-timestep integration, zero gimbal lock across pitch loops, smooth yaw/roll damping, and reset mechanics verified across 32 unit tests.
2. **Procedural Geometry & Collision:** Heightfield determinism, surface consistency, chunk boundaries, and terrain collision verified across 64 unit tests.
3. **Environment & Visual Systems:** Vegetation density/biome exclusivity, cloud drift bounds, camera smoothing, speed lines, and audio model math verified across 51 unit tests.
4. **Combat & AI:** Gun cooldowns, pool exhaustion/reuse, swept anti-tunneling, homing turn-rate clamps, FSM state transitions, terrain pull-ups, and mission lifecycle verified across 55 unit tests.
5. **No Compiler Warnings:** Clean compilation with `-Wall -Wextra -Wpedantic -Werror -std=c++20`.

### What Remains Unverified on Real Windows Hardware
The following items require compilation and runtime validation on a physical Windows x64 system with the DXLib SDK:
1. **MSVC SDK Linking:** Linking against `DxLib.h` and `DxLib_x64.lib` using MSVC autolink pragmas and `/MT` / `/MTd` runtimes.
2. **DirectX 3D Rendering:**
   - Player mesh (blue/silver) vs. hostile enemy meshes (crimson/charcoal livery).
   - Yellow tracer bullet lines and missile meshes with expanding smoke trails.
   - Screen-space 2D radar blips and lock diamond reticles.
   - HUD text and mission banner rendering via `DrawString` and `DrawBox`.
3. **DXLib Audio Playback:**
   - Note: The repository does not ship with `.wav` audio files. The audio model safely operates as a no-op when sound handles are unassigned (`-1`). Real sound effects (gunfire, explosions, missile launch, lock-on beep) require loading valid audio assets.
4. **Windows OS Events:**
   - Window resize, focus loss/recovery (neutral input), and raw keyboard polling through `GetHitKeyStateAll`.

---

## Recommended Next Steps

1. **Windows DXLib Acceptance Run:** Clone the repository onto a Windows x64 machine with Visual Studio 2022, configure `DXLIB_ROOT`, build `SkyCombat.exe`, and verify 60 FPS flight, gun/missile firing, enemy dogfights, and HUD rendering.
2. **Audio Asset Integration:** Place `.wav` sound files in an `assets/sounds/` directory and bind them to the combat events (`onGunFired`, `onMissileLaunched`, `onExplosion`, `onLockAcquired`).
