# Flight Tuning & Camera Controls (M1–M4)

| Parameter | Value | Notes |
|---|---:|---|
| Minimum speed | 25 m/s (90 km/h) | Airspeed lower clamp |
| Spawn / cruise speed | 70 m/s (252 km/h) | Default level flight |
| Maximum speed | 140 m/s (504 km/h) | Airspeed upper clamp |
| Pitch rate | 65 degrees/s | Local X axis |
| Yaw rate | 55 degrees/s | World Y axis |
| Maximum visual bank | 50 degrees | Visual roll mesh angle |
| Bank auto-level response | 3.5 per second | Exponential decay $1 - e^{-3.5 \cdot \Delta t}$ |
| Acceleration / deceleration | 25 / 30 m/s² | Throttle response rate |
| Safe terrain clearance | 8 m | Critical warning threshold |
| Warning terrain clearance | 25 m | Pitch assist trigger zone |
| Camera ground margin | 2.5 m | Minimum camera height above terrain |

---

## M4 Camera Polish Parameters

| Parameter | Value | Behavior |
|---|---:|---|
| Positional turn lag | $-1.8 \cdot \text{visualBank} \cdot \text{right}$ | Displaces camera laterally opposite bank during turns |
| Camera roll fraction | 0.20 (20%) | Camera tilts by 20% of aircraft bank |
| Camera roll hard cap | 12.0 degrees ($0.2094\,\text{rad}$) | Limits camera bank to preserve horizon orientation |
| Camera FOV range | 65–80 degrees | Linearly mapped across speed range $[25, 140]\,\text{m/s}$ |
| Position smoothing response | 6.0 per second | Exponential smoothing $1 - e^{-6.0 \cdot \Delta t}$ |
| Look-at target response | 8.0 per second | Exponential smoothing $1 - e^{-8.0 \cdot \Delta t}$ |
| FOV smoothing response | 4.0 per second | Exponential smoothing $1 - e^{-4.0 \cdot \Delta t}$ |
| Up-vector response | 8.0 per second | Exponential smoothing with view-axis projection |
| Terrain avoidance | Active (M2) | Clamps camera position $Y \ge \text{GroundHeight} + 2.5\,\text{m}$ |

---

## M4 Speed Effects & Audio Tuning

| Parameter | Value | Behavior |
|---|---:|---|
| Speed line threshold | 105.0 m/s (378 km/h) | Zero intensity below 105 m/s, ramps linearly to 1.0 at 140 m/s |
| Speed line boost bonus | +0.20 intensity | Adds instant cue upon boost key engagement |
| Speed line toggle | Configurable (default on) | Suppresses radial lines completely when disabled |
| Engine audio volume | 0.35–1.00 | Monotonically ramps with speed and throttle |
| Engine audio pitch | 0.80–1.60 (up to 1.80 boost) | Pitch multiplier mapped to engine RPM |
| Wind audio volume | 0.00–1.00 | Ramps quadratically above 35 m/s, roaring at max speed |
