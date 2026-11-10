# M1 flight tuning

| Parameter | Value |
|---|---:|
| Minimum / spawn cruise / maximum speed | 25 / 70 / 140 m/s |
| Pitch / world-up yaw | 65 / 55 degrees/s |
| Maximum visual bank | 50 degrees |
| Bank response (including release) | 3.5 per second |
| Acceleration / deceleration (M1 choice) | 25 / 30 m/s² |
| Camera position / target / FOV response | 6 / 8 / 4 per second |
| Camera FOV | 65–80 degrees |
| Spawn | (0, 120, 0) metres, forward +Z |

Throttle adjusts speed linearly and clamps it. Release holds current speed.
Yaw is applied about world +Y; pitch about the resulting local right axis.
Position advances by updated forward × speed × fixed delta. Pitch has no
leveling assistance. Visual roll approaches `-turn * maxBank` using
`1 - exp(-3.5 * dt)`; a right turn therefore lowers the right wing.

Camera offset is 24–30 m behind and 8 m along physical up. Look-ahead is 18–38 m.
Offsets and FOV use normalized speed across 25–140 m/s. Position, target and FOV
smooth exponentially with separate rates. There is no additional camera bank.

The section 5 clearance bands (8/25 m), avoidance, collision and 2 km bounds
require M2 terrain/world data and are intentionally deferred. The M1 grid is not
an implicit collision surface. Health 100 is reserved state only.
