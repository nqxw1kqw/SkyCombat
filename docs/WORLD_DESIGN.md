# SkyCombat World & Terrain Design (M2)

This document describes the world geometry, procedural heightfield synthesis, collision/mesh consistency, and terrain-aware flight rules implemented for Milestone M2.

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
-X (West: Meadows)   │      Center       │   +X (East: Meadows)
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
- **East ($+X$) & West ($-X$):** Gentle rolling meadowlands prepared for M3 floral/biome coloration.

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
