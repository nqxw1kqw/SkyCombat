# SkyCombat Architecture

## Overview & Layering

SkyCombat is structured with a strict separation between the **pure simulation/logic layer** and the **DXLib platform/presentation layer**:

```
+-------------------------------------------------------------------------+
|                              Game (core/Game)                           |
|       (OS events, main loop, accumulator, input polling, DXLib render)  |
+--------------------+---------------------+--------------------+---------+
                     |                     |                    |
        +------------v-----------+         |          +---------v---------+
        |   Pure Simulation      |         |          | Presentation /    |
        |   (Fixed 60 Hz Step)   |         |          | Interpolated Draw |
        +------------------------+         |          +-------------------+
        | - Aircraft / FlightCtrl|         |          | - AircraftRenderer|
        | - WorldCollision       |         |          | - TerrainRenderer |
        | - TargetingSystem      |         |          | - Radar (Draw)    |
        | - WeaponSystem         |         |          | - HUD (Draw)      |
        | - EnemyAI / Steering   |         |          | - SpeedEffects    |
        | - MissionSystem        |         |          | - AudioModel      |
        +------------------------+         |          +-------------------+
                     |                     |
        +------------v---------------------v----------+
        |                Pure Foundation              |
        |  (Math, Transform, Noise, TerrainSystem,   |
        |   BiomeSystem, CloudSystem, Vegetation)     |
        +---------------------------------------------+
```

1. **Pure Layer (`src/core/Math.h`, `Transform.h`, `aircraft/`, `world/`, `effects/`, `audio/`, `combat/`, `ai/`, `ui/Radar.h` (data model), `ui/HUD.h` (data model)):**
   - Zero DXLib or Windows headers.
   - Built with standard C++20 (`std::vector`, `std::array`, `<cmath>`, `<algorithm>`).
   - 100% testable via unit test suites (`tests/run_portable.sh`) on any platform.
2. **DXLib Layer (`src/core/Game.cpp`, `main.cpp`, `rendering/`, `ui/HUD.cpp` (drawing), `ui/Radar.cpp` (drawing)):**
   - Encapsulates all DXLib calls (`DrawPolygonIndexed3D`, `DrawLine3D`, `DrawString`, `PlaySoundMem`, etc.).
   - Guarded by `#if !defined(SKY_PORTABLE) && defined(_WIN32)`.
   - Never mutates simulation state; only reads interpolated aircraft transforms and immutable system data models.

---

## Combat & AI Data Flow (M5)

During each fixed simulation step ($60\,\text{Hz}, \Delta t \approx 0.0167\,\text{s}$):

```
                       +-------------------+
                       |    InputState     |
                       | (fireGun, missile,|
                       |   switchTarget)   |
                       +---------+---------+
                                 |
                                 v
+------------------+   +-------------------+   +--------------------+
| Player Aircraft  |-->|  TargetingSystem  |<--| Living Enemy List  |
| (pos, forward)   |   | (cone, range, lock|   | (positions, status)|
+--------+---------+   |  acquire timer)   |   +---------+----------+
         |             +---------+---------+             |
         |                       |                       |
         |  fireGun / missile    | lockedTargetIndex     |
         +-----------------+     |                       |
                           |     v                       v
                      +----v--------------------------------+
                      |          WeaponSystem               |
                      |  - Swept bullet ray-sphere tests    |
                      |  - Bullet vs terrain (GroundHeight) |
                      |  - Homing missile rate-limited steer|
                      |  - Proximity detonation & damage    |
                      +------------------+------------------+
                                         |
                                         | CombatEvents (hits, kills)
                                         v
                      +-------------------------------------+
                      |           MissionSystem             |
                      |  (Active -> Cleared / Failed)       |
                      +------------------+------------------+
                                         |
                                         v
+------------------+   +------------------------------------+
|  EnemyAI (4x)    |-->|              Radar / HUD           |
|  - FSM Update    |   |  - Pure RadarData blip projection  |
|  - Threat Evade  |   |  - HUD HP, targets remaining,      |
|  - Ground Height |   |    lock status, mission banners    |
|  - Assisted Steer|   +------------------------------------+
+------------------+
```

### 1. Targeting Pipeline (`src/combat/TargetingSystem.h/.cpp`)
- Evaluates enemies within maximum lock range ($1200\,\text{m}$) and view cone ($30^\circ$ half-angle from player's nose vector).
- `update(dt, playerPos, playerForward, enemies)`:
  - When a target enters the cone, it begins `Acquiring`.
  - After maintaining cone/range lock for $0.8\,\text{s}$, state transitions to `Locked`.
  - If the target leaves the cone or dies, state reverts to `Searching` or `NoTarget`.
- `cycleTarget(playerPos, playerForward, enemies)`:
  - Triggered by `Tab` key press.
  - Finds all living enemies in the forward hemisphere, sorts them by distance, and selects the next in sequence.

### 2. Weapons & Anti-Tunneling Swept Collision (`src/combat/WeaponSystem.h/.cpp`)
- **Machine Gun:**
  - Fire rate: $0.08\,\text{s}$ cooldown ($12.5\,\text{rounds/sec}$), speed $600\,\text{m/s}$, lifetime $2.0\,\text{s}$, damage $25\,\text{HP}$.
  - Uses an inactive-first pooled allocation of 120 bullets.
  - **Anti-Tunneling Swept Sphere Check:**
    Instead of discrete point collision, each bullet step tests the segment $[P_0, P_1]$ against each target sphere $(C, R)$:
    $$\mathbf{v} = P_1 - P_0, \quad t = \mathrm{clamp}\left(\frac{(C - P_0) \cdot \mathbf{v}}{\mathbf{v} \cdot \mathbf{v}}, 0, 1\right), \quad Q = P_0 + t\mathbf{v}$$
    $$\text{Hit if } \|Q - C\| \le R_{\text{hit}} \quad (R_{\text{hit}} = 3.5\,\text{m})$$
    This guarantees bullets traveling $30\,\text{m}$ per tick never skip past a $7\,\text{m}$ target sphere.
  - Terrain collision checks $P_1.y \le \mathrm{GroundHeight}(P_1.x, P_1.z)$.

### 3. Homing Missile Dynamics (`src/combat/Missile.h/.cpp`)
- Launched via `F` key with confirmed lock.
- **Rate-Limited Steering:**
  The missile computes the desired direction toward the target:
  $$\mathbf{d} = \mathrm{normalize}(P_{\text{target}} - P_{\text{missile}})$$
  Angular error $\theta = \arccos(\mathrm{clamp}(\mathbf{f} \cdot \mathbf{d}, -1, 1))$.
  The turn per step is clamped:
  $$\Delta \theta = \min(\theta, \omega_{\max} \Delta t) \quad (\omega_{\max} = 45^\circ/\text{s} \approx 0.7854\,\text{rad/s})$$
  The missile rotates toward $\mathbf{d}$ around $\mathbf{f} \times \mathbf{d}$, ensuring visible curved trajectories rather than instantaneous snapping.
- **Detonation & Damage:**
  Proximity radius $6.0\,\text{m}$ deals $100\,\text{HP}$ lethal damage. Emits explosion combat events.

### 4. Enemy AI Finite State Machine (`src/ai/EnemyAI.h/.cpp`, `SteeringBehavior.h/.cpp`)
Each enemy plane possesses an `Aircraft` state, health ($100\,\text{HP}$), and `FlightController`.
The FSM executes the following state transitions:
- `PATROL`: Cruises between tactical waypoints. Transitions to `CHASE` when player distance $< 1000\,\text{m}$.
- `CHASE`: Pursues player position using coordinated yaw/bank steering. Transitions to `ATTACK` when distance $< 350\,\text{m}$ and angle $< 25^\circ$.
- `ATTACK`: Fires nose cannon at player with $0.4\,\text{s}$ cooldown. Breaks off if player flies out of cone or range.
- `EVADE`: Triggered when player acquires a missile lock on the enemy, when a missile is actively tracking it, or upon taking weapon damage. AI pulls lateral break turns and weaves.
- `RETREAT`: Triggered when health drops below $25\%$. AI flees toward distant map boundaries at full throttle ($120\,\text{m/s}$).
- **Terrain Clearance Safety Override:**
  Before applying desired steering, AI evaluates ground height. If altitude above ground $< 35\,\text{m}$, pitch is forced upward to prevent CFIT (controlled flight into terrain).
- **World Boundary Damping:**
  Steering blends toward map center when approaching boundary radius ($1000\,\text{m}$).

### 5. Mission State Lifecycle (`src/combat/MissionSystem.h/.cpp`)
- Tracks living enemy count.
- Transitions to `Cleared` when all enemies reach $0\,\text{HP}$.
- Transitions to `Failed` if player $HP \le 0$ or player crashes into terrain.
- Resets synchronously on `R` key.

### 6. Radar & HUD Integration (`src/ui/Radar.h/.cpp`, `HUD.h/.cpp`)
- **Radar Model:**
  Pure 2D polar projection:
  $$\Delta \mathbf{p} = \mathbf{p}_{\text{target}} - \mathbf{p}_{\text{player}}$$
  Rotated into player's horizontal frame of reference (heading):
  $$x_{\text{radar}} = \frac{\Delta \mathbf{p} \cdot \mathbf{right}}{R_{\max}}, \quad y_{\text{radar}} = \frac{\Delta \mathbf{p} \cdot \mathbf{forward}_{\text{horiz}}}{R_{\max}}$$
  Yields normalized blip positions in $[-1, 1]$ screen space.
- **HUD Model:**
  Computes health percentages, remaining target count, target distance, lock state strings, and win/loss banner data. The DXLib layer performs pure string and polygon rendering with zero logic side effects.
