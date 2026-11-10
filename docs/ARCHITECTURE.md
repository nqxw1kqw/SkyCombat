# M1 architecture

`core/Math.h` and `Transform.h` define double-precision vectors and normalized
quaternions. Coordinates are metres: +X right, +Y up, +Z forward. The mesh nose is
+Z. Positive world-Y yaw turns right; negative local-X pitch raises the nose.
No Euler angles are accumulated, so loops have no pitch clamp or gimbal lock.

Pure modules: `Time` (steady clock, accumulator), `Input` (boolean keyboard mapping
and reset edge), `Aircraft` (state/reset/interpolation), `FlightController`
(steering/speed/integration), and `ChaseCamera` (pose/FOV). None includes DXLib.
`core/Game` is the app orchestration boundary; it and `main` own DXLib lifecycle,
keyboard polling and drawing. `rendering/AircraftRenderer` accepts immutable state
and draws a code-built triangle mesh. Grid rendering is app glue, not a terrain
system. There are no dependencies on future modules.

Frame order:

1. Process OS messages; sample keyboard (neutral when unfocused).
2. Read steady elapsed time and accumulate at most 0.25 s.
3. Map input and latch reset/quit; consume fixed 1/60 s updates. Each update
   snapshots previous state, steers, updates speed, then integrates position.
4. Interpolate previous/current transform, speed and visual bank by remainder.
5. Smooth camera from the interpolated aircraft with clamped render delta.
6. Clear, set depth/culling/lighting and camera/FOV, draw grid and aircraft,
   disable depth for debug text, flip the back buffer.

Render does not mutate aircraft state. Reset clears accumulator and snaps both
simulation snapshots and camera, preventing a long return sweep. Mapping occurs
once per OS poll and held axes are reused by all fixed steps; reset is processed
at poll cadence so presses on frames without simulation steps are retained.

Physical orientation excludes visual bank. Mesh roll is applied locally around
+Z, and never feeds flight or camera. The camera follows physical up through loops
without visual-bank roll; its smoothed up is projected perpendicular to view with
a degeneracy fallback. At a fixed target, exponential smoothing is independent
of render cadence; moving-target tracking is sampled at render cadence.
