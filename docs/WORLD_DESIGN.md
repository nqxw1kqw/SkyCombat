# SkyCombat World & Terrain Design (M2 / M3)

This document describes the world geometry, procedural heightfield synthesis, collision/mesh consistency, terrain-aware flight rules, biome palette, sky/fog parameters, and chunking/LOD architecture.

---

## 1. World Dimensions and Coordinate Conventions

- **World dimensions:** 2.0 km × 2.0 km ($X \in [-1000, 1000]\,\text{m}, Z \in [-1000, 1000]\,\text{m}$).
- **Coordinate system:**
  - $+X$: East / Right
  - $+Y$: Up / Altitude (metres)
  - $+Z$: North / Forward
- **Grid resolution:** $80 \times 80$ quad cells, $81 \times 81$ grid vertices, cell size $25\,\text{m}$.
- **Mesh topology:** 12,800 flat-shaded triangles (2 triangles per cell split along the $(0,0) \to (1,1)$ diagonal).
- **Spawn point:** $(0, 120, 0)$ heading North ($+Z$), placed over safe, open grassland with $>100\,\text{m}$ initial clearance.

---

## 2. Procedural Heightfield Formula

Elevation is computed using a deterministic fixed seed (`seed = 1337`) with 2D quintic-Hermite gradient noise and fractal summation:

$$H(x, z) = \mathrm{Hills}(x, z) + \mathrm{MountainMask}(x, z) \cdot \mathrm{Mountains}(x, z) + \mathrm{Detail}(x, z) - \mathrm{ValleyMask}(x, z) \cdot \mathrm{ValleyDepth}(x, z)$$

A safety floor ensures $H(x, z) \ge 2.0\,\text{m}$ across all coordinates.

### Component Functions

1. **$\mathrm{Hills}(x, z)$:**
   - Base rolling topography across the world.
   - Base elevation: $18\,\text{m}$.
   - 3-octave Fractal Brownian Motion (FBM) with spatial frequency $0.003\,\text{m}^{-1}$ and amplitude $18\,\text{m}$.
   - Center flattener: smoothstep attenuation between radial distances $60\,\text{m}$ and $300\,\text{m}$ from the origin $(0, 0)$ ensures the central airfield and dogfight area is flat and obstacle-free.

2. **$\mathrm{MountainMask}(x, z)$:**
   - Active in the northern sector ($Z > 80\,\text{m}$), smoothly transitioning to full mountain presence by $Z = 450\,\text{m}$ via $\mathrm{smoothstep}(80, 450, z)$.
   - Edge taper: smooth falloff within $150\,\text{m}$ of the northern and lateral world boundaries ($\mathrm{smoothstep}(1000, 850, |x|)$ and $\mathrm{smoothstep}(1000, 850, z)$) to prevent jarring boundary cliffs.

3. **$\mathrm{Mountains}(x, z)$:**
   - 4-octave ridged multifractal noise with frequency $0.0025\,\text{m}^{-1}$.
   - Produces prominent sharp ridges and peaks reaching $140\text{--}200\,\text{m}$ altitude in the North.

4. **$\mathrm{Detail}(x, z)$:**
   - 2-octave high-frequency noise with spatial frequency $0.025\,\text{m}^{-1}$ and $3.0\,\text{m}$ amplitude, adding subtle low-poly facet variation.

5. **$\mathrm{ValleyMask}(x, z)$:**
   - Active in the southern sector ($Z < -50\,\text{m}$), fully developed south of $Z = -250\,\text{m}$ via $\mathrm{smoothstep}(-50, -250, z)$.
   - Carves a winding canyon following the sinusoidal trajectory:
     $$x_{\text{valley}}(z) = 180 \cdot \sin(0.006 \cdot z) + 70 \cdot \cos(0.014 \cdot z)$$
   - Cross-sectional profile: $\mathrm{smoothstep}(120, 30, |x - x_{\text{valley}}(z)|)$, creating a ~140 m wide canyon corridor suitable for high-speed low-altitude flight.
   - Tapers smoothly toward the southern and lateral boundaries.

6. **$\mathrm{ValleyDepth}(x, z)$:**
   - Carving depth between $20\,\text{m}$ and $30\,\text{m}$, modulated by 2-octave low-frequency noise.

---

## 3. World Layout Overview

```text
               +Z (North: High Mountains 150-200m)
                     ┌───────────────────┐
                     │   ▲▲▲▲▲   ▲▲▲▲▲   │
                     │  ▲▲▲▲▲▲▲ ▲▲▲▲▲▲▲  │
                     │   ▲▲▲▲▲   ▲▲▲▲▲   │
-X (West: Purple)    │      Center       │   +X (East: Pink)
                     │     Grassland     │
                     │  Spawn (0,120,0)  │
                     │                   │
                     │    \         /    │
                     │     \  Valley /   │
                     │      \_______/    │
                     └───────────────────┘
               -Z (South: Winding Canyon 2-10m)
```

- **Center (0, 0):** Flat grassland plateau ($Y \approx 18\,\text{m}$), safe spawn at $Y = 120\,\text{m}$ ($>100\,\text{m}$ clearance).
- **North ($+Z > 100\,\text{m}$):** High mountain peaks and elevated ridge corridors ($Y \approx 120\text{--}200\,\text{m}$).
- **South ($-Z < -100\,\text{m}$):** Deep winding canyon floor ($Y \approx 2\text{--}10\,\text{m}$) flanked by $20\text{--}30\,\text{m}$ ridges.
- **West ($-X < -150\,\text{m}$):** Purple flower fields (lavender blossom).
- **East ($+X > 150\,\text{m}$):** Pink flower fields (rose blossom).

---

## 4. Triangle-Consistent Collision & Render Parity

To eliminate visual/collision discrepancies (where aircraft collide with invisible geometry or penetrate the visible ground), `TerrainSystem` implements **triangle-consistent plane interpolation**:

1. For any coordinate $(x, z)$, the containing cell $(i_x, i_z)$ is identified:
   $$u = \frac{x - x_{\min}}{\text{cellSize}}, \quad v = \frac{z - z_{\min}}{\text{cellSize}}$$
2. Each quad is split into two triangles along the $(0,0) \to (1,1)$ diagonal:
   - **Triangle 0 ($f_x \ge f_z$):** Vertices $(v_{00}, v_{10}, v_{11})$.
     $$H(x, z) = h_{00} + f_x (h_{10} - h_{00}) + f_z (h_{11} - h_{10})$$
   - **Triangle 1 ($f_x < f_z$):** Vertices $(v_{00}, v_{11}, v_{01})$.
     $$H(x, z) = h_{00} + f_x (h_{11} - h_{01}) + f_z (h_{01} - h_{00})$$
3. Because the rendered DXLib mesh uses the exact same vertex heights and triangulation, `groundHeight(x, z)` matches the exact planar surface of the rendered polygon down to floating-point precision ($< 10^{-12}\,\text{m}$).

---

## 5. Terrain-Aware Flight and Clearance Rules

`FlightController` evaluates clearance $C = Y_{\text{aircraft}} - \text{GroundHeight}(X, Z)$:

- **Clearance $> 25\,\text{m}$:** Normal arcade flight. No assist intervention.
- **Clearance $8\text{--}25\,\text{m}$:**
  - If climbing or level ($\text{forward}.y \ge 0$): No intervention.
  - If descending ($\text{forward}.y < 0$): Applies limited pitch-up assist proportional to descent rate and proximity to $8\,\text{m}$:
    $$\text{assistPitchRate} = \left(\frac{25 - C}{25 - 8}\right) \cdot (-\text{forward}.y) \cdot \omega_{\max}$$
    Does not teleport or force automatic terrain following; pilot retains pitch authority.
- **Clearance $< 8\,\text{m}$:** Low clearance warning state active (`terrainWarning = true`). Assist continues if descending, but collision remains possible.
- **Ground collision ($C \le 0$):**
  - Aircraft enters consistent crash state: `crashed = true`, `speed = 0`.
  - Aircraft altitude is clamped to `groundHeight(x, z)` (never sinks below the mesh).
  - Pressing `R` restores flight to the spawn point.
- **World boundary handling:**
  - Soft-turn assist engages within $100\,\text{m}$ of borders ($|x|, |z| > 900\,\text{m}$) when heading outward, smoothly steering the aircraft back toward the center.
  - Hard clamp at $|x| \le 1000\,\text{m}, |z| \le 1000\,\text{m}$ prevents flying out of the world.

---

## 6. Chase Camera Ground Avoidance

`ChaseCamera` applies ground obstruction avoidance:
- Whenever the camera's smoothed target position drops below `groundHeight(x, z) + 2.5 m`, its $Y$ position is clamped to `groundHeight + 2.5 m`.
- View and up vectors are re-orthogonalized, preventing the camera from ever clipping under hills or canyon ridges.

---

## 7. Biome System & Palette Reference (M3)

The `BiomeSystem` classifies every point on the terrain based on position $(x, z)$, height $y$, and surface slope normal $N$, with smooth $C^1$ weight blending across boundaries.

| Biome | Primary Region | Defining Criteria | Base Color (RGB / Hex) | Notes |
|---|---|---|---|---|
| **Grassland** | Center ($|X| < 150\text{ m}, |Z| < 200\text{ m}$) | Low elevation ($Y \le 35\text{ m}$), flat slope | $(0.55, 0.77, 0.45)$ / `#8cc473` | Spring green airfield |
| **PurpleFlowerField** | West ($-X < -120\text{ m}$) | Mid-elevation, moderate slope, $|Z| < 400\text{ m}$ | $(0.74, 0.58, 0.89)$ / `#bc94e2` | Lavender blossom (matches `terrain.obj`) |
| **PinkFlowerField** | East ($+X > 120\text{ m}$) | Mid-elevation, moderate slope, $|Z| < 400\text{ m}$ | $(0.93, 0.71, 0.79)$ / `#edb5c9` | Rose blossom (matches `terrain.obj`) |
| **Highland** | Rolling ridges ($Y \in [35, 85\text{ m}]$) | Moderate elevation, gentle slope | $(0.58, 0.69, 0.48)$ / `#94b07a` | Upland olive meadow |
| **MountainRock** | North ($Z > 100\text{ m}$) / cliffs | $Y > 85\text{ m}$ or steep slope ($N_y < 0.82$) | $(0.58, 0.61, 0.56)$ / `#949c8f` | Slate rock; peaks $>160\text{ m}$ blend to snow `#d1d9e0` |
| **ValleyFloor** | South canyon ($Z < -50\text{ m}$) | Valley mask $>0$, $Y \le 20\text{ m}$ | $(0.78, 0.68, 0.50)$ / `#c7ad80` | Sandy canyon riverbed |

### Smooth Biome Blending
Weights $w_i(x, z, y, N)$ are calculated using cubic smoothstep transitions and normalized so $\sum w_i = 1.0$. The final vertex/triangle color is:
$$C = \sum_{i} w_i C_i$$
This prevents visible tile seams or abrupt color jumps across chunk or biome boundaries.

---

## 8. Sky, Sun & Distance Fog Model (M3)

`SkySystem` provides pure lighting, sky dome, and atmospheric fog parameters matching `docs/palette_reference.png`:

- **Zenith color:** $(0.28, 0.58, 0.90)$ (`#4794e6`) — bright azure daytime sky dome.
- **Horizon color:** $(0.70, 0.84, 0.95)$ (`#b2d6f2`) — pale hazy horizon blue.
- **Sun direction:** Normalized $(0.4, 0.8, 0.3)$ — high-angle direct sunlight.
- **Sun color:** $(1.0, 0.98, 0.90)$ — warm golden sunlight.
- **Distance fog:**
  - Fog color matches the horizon color $(0.70, 0.84, 0.95)$, causing distant geometry to blend seamlessly into the sky.
  - Near distance: $500\,\text{m}$ (crystal-clear foreground combat).
  - Far distance: $3500\,\text{m}$ (full atmospheric blend before camera far plane).
  - Fog factor: linear interpolation between near and far distance.

---

## 9. Terrain Chunking & 3-Level LOD Scheme (M3)

The $2000\,\text{m} \times 2000\,\text{m}$ world is partitioned into a $4 \times 4$ grid of 16 independent chunks, each $500\,\text{m} \times 500\,\text{m}$:

| LOD Level | Cell Size | Cells per Chunk | Vertices per Chunk | Triangles per Chunk | Total World Triangles (16 Chunks) |
|---|---|---|---|---|---|
| **LOD 0 (High)** | $25\,\text{m}$ | $20 \times 20$ | $21 \times 21 = 441$ | $800$ | $12,800$ (exact parity with M2) |
| **LOD 1 (Medium)**| $50\,\text{m}$ | $10 \times 10$ | $11 \times 11 = 121$ | $200$ | $3,200$ |
| **LOD 2 (Low)** | $100\,\text{m}$| $5 \times 5$ | $6 \times 6 = 36$ | $50$ | $800$ |

### Distance-Based LOD with Hysteresis
To prevent visual popping (LOD thrashing when camera hovers near a threshold distance), chunks maintain their current LOD and switch based on separate expansion/contraction thresholds:

- **LOD 0 $\leftrightarrow$ LOD 1:**
  - Distance $> 650\,\text{m} \implies$ switch from LOD 0 to LOD 1.
  - Distance $< 550\,\text{m} \implies$ switch from LOD 1 to LOD 0.
  - Buffer zone: $100\,\text{m}$ hysteresis margin.
- **LOD 1 $\leftrightarrow$ LOD 2:**
  - Distance $> 1200\,\text{m} \implies$ switch from LOD 1 to LOD 2.
  - Distance $< 1100\,\text{m} \implies$ switch from LOD 2 to LOD 1.
  - Buffer zone: $100\,\text{m}$ hysteresis margin.

### Edge Parity Between Adjacent Chunks
Along any shared chunk boundary (e.g. $X = 0\,\text{m}$ between chunk $(1, cz)$ and chunk $(2, cz)$), boundary vertices at the same LOD evaluate the identical coordinate $(X, Z)$ against the pure `IHeightProvider` and `BiomeSystem`. This guarantees identical elevation and color along the seam, eliminating cracks.

---

## 10. Distant Mountain Silhouettes (M3)

Beyond the northern boundary ($Z \in [1050, 1700]\,\text{m}$), a low-poly proxy ridge of 8 distinct mountain peaks ($Y \approx 180\text{--}350\,\text{m}$) is generated as lightweight pure mesh data (~30 triangles). Colored in atmospheric slate blue (`#808c9e`), these silhouettes rise above the horizon and fade gracefully into the distance fog without requiring complex terrain streaming.

---

## 11. Environment Art: Batched Vegetation & Cloud Clusters (M4)

### 11.1 Deterministic Vegetation System (`VegetationSystem`)
Vegetation is deterministically scattered using a fixed seed (`seed = 1337`) and merged into static vertex/index buffers grouped by category to eliminate individual draw calls:

1. **Grass Clumps (`grassBatch`):**
   - Distributed across Grassland and Highland biomes.
   - Density cap: up to $1,200$ clumps (`WorldConfig::maxGrassClumps`).
   - Geometry: crossed double-quad mesh (8 vertices, 4 triangles, 2 quads along X and Z).
   - Colors: dark forest base (`#3e6b22`), vivid spring green tips (`#82c35f`).

2. **Purple Flower Tufts (`purpleFlowersBatch`):**
   - Strictly confined to the Western purple flower field ($X < 0\,\text{m}$, predominant biome `PurpleFlowerField`, weight $> 0.30$).
   - Density cap: up to $400$ tufts (`WorldConfig::maxPurpleFlowerTufts`).
   - Colors: fresh foliage base (`#478028`), soft lavender petals (`#bc94e2`).

3. **Pink Flower Tufts (`pinkFlowersBatch`):**
   - Strictly confined to the Eastern pink flower field ($X > 0\,\text{m}$, predominant biome `PinkFlowerField`, weight $> 0.30$).
   - Density cap: up to $400$ tufts (`WorldConfig::maxPinkFlowerTufts`).
   - Colors: fresh foliage base (`#478028`), rose blossom petals (`#edb5c9`).

4. **Strict Terrain Exclusions:**
   - **Steep slopes:** Excluded wherever surface normal $N_y < 0.85$ (no foliage clinging to cliffs).
   - **Rock & high altitude:** Excluded where $Y > 85\,\text{m}$ or mountain rock weight $> 0.05$.
   - **Valley floor path:** Excluded from the canyon bed where valley mask $> 0.05$ or valley floor weight $> 0.05$.
   - **Surface attachment:** Clump bases snap exactly to $Y = \text{GroundHeight}(X, Z)$.

### 11.2 Deterministic Low-Poly Cloud System (`CloudSystem`)
Atmospheric cloud blobs populate the sky above the playable airspace:

1. **Cluster Architecture:**
   - 18 cloud clusters (`WorldConfig::numCloudClusters`) generated deterministically (`seed = 1337`).
   - Altitude band: strictly within $[230, 360]\,\text{m}$ (`cloudMinAltitude` to `cloudMaxAltitude`).
   - Geometry: cluster of flattened overlapping low-poly boxes with shaded slate undersides (`#c4d2e2`), soft rims (`#e2ebf5`), and puffy white upper facets (`#f8fbff`).

2. **Drift & Seamless Wrap:**
   - Clusters drift slowly eastward along $+X$ at $2.5\,\text{m/s}$ (`cloudDriftSpeed`).
   - Seamless wrapping: upon crossing $X = +1200\,\text{m}$, clusters wrap to $-1200\,\text{m}$.
   - **Zero per-frame allocation:** Cloud vertex positions are adjusted in-place during `update(dt)` using precomputed cluster vertex index spans; no buffers are reallocated or cleared at runtime.
