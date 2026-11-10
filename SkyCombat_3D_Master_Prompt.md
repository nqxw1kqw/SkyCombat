# SK YCOMBAT 3D — MASTER IMPLEMENTATION PROMPT

> **Vai trò:** Bạn là Senior C++ Game Engine / Gameplay / Rendering Engineer. Hãy xây dựng một game 3D low-poly arcade dogfight tên **SkyCombat** bằng **C++20 + DXLib** trên Windows, theo kế hoạch dưới đây. Đây là **đặc tả và prompt triển khai**, không phải tuyên bố rằng các tính năng đã hoàn thành.

## 0. Bối cảnh và mục tiêu

- Game 3D máy bay chiến đấu góc nhìn thứ ba; điều khiển **Assisted Arcade** (tự cân bằng), không mô phỏng khí động học thực tế.
- Tham chiếu video gameplay người dùng đã cung cấp: cảm giác bay nhanh sát địa hình, đồi cỏ xanh, cánh đồng hoa hồng/tím, thung lũng, núi xa, mây lớn, màu tươi, camera mượt, cảm giác tốc độ. **Không sao chép tài sản, nhân vật, tên hoặc nội dung có bản quyền từ video.** Nếu agent không có quyền truy cập video, dựa vào mô tả này và hỏi người dùng nếu cần xác minh chi tiết.
- Giữ gameplay dogfight: súng máy, tên lửa, lock-on, AI truy đuổi/né tránh; nhưng **ưu tiên xây World + Flight trước Combat**.
- Nền tảng: Windows x64, Visual Studio 2022, CMake, C++20, DXLib. Không chuyển sang Unity/Unreal hoặc thay DXLib nếu không được yêu cầu.
- Phong cách: low-poly, có thể dựng mesh máy bay/terrain trực tiếp bằng code ban đầu; không phụ thuộc asset bên ngoài cho bản prototype.
- Đơn vị thế giới: 1 unit = 1 mét, trục Y hướng lên; quy ước forward của máy bay và hệ tọa độ cần xác định một lần, ghi rõ và nhất quán.

## 1. Đầu ra bắt buộc của coding agent

1. Project hoàn chỉnh có `CMakeLists.txt`, `README.md`, source `.h/.cpp`, assets placeholder (nếu cần) và hướng dẫn build Visual Studio/DXLib SDK.
2. Mỗi milestone phải build được độc lập; không để placeholder TODO ở đường đi bắt buộc chạy game.
3. Code thật, không chỉ pseudocode; cập nhật README với phím điều khiển, cấu trúc, thông số và giới hạn.
4. Cung cấp log/ảnh/chứng cứ build nếu môi trường có DXLib và Windows; **không khẳng định đã build/chạy nếu chưa kiểm chứng**.
5. Khi API DXLib chưa chắc chắn, tra header/SDK thực tế trước khi sử dụng; tránh giả định API hỗ trợ tính năng đồ họa nâng cao.
6. Tách gameplay khỏi DXLib renderer càng nhiều càng tốt; ưu tiên lớp toán học và logic thuần C++ dễ test.
7. Báo cáo file đã thay đổi, cách chạy, kết quả kiểm thử, vấn đề còn tồn tại, bước tiếp theo.

## 2. Phạm vi và thứ tự ưu tiên

### Milestone M1 — Flight Prototype (P0)
- Window 1280x720, double buffering, input keyboard, game loop, fixed update 60 Hz, delta time render và FPS debug.
- `Transform`: position, orientation (quaternion hoặc basis/ma trận chuẩn hóa), forward/right/up; tránh lỗi gimbal lock khi lộn vòng về sau.
- `Aircraft`: position, speed, health placeholder, flight state.
- `FlightController`: Pitch W/S, Turn A/D, throttle Shift/Ctrl, assisted banking, auto-level khi thả phím; R reset, Esc thoát.
- `ChaseCamera`: smooth follow độc lập FPS, hướng nhìn, camera offset, điều chỉnh FOV theo speed.
- Máy bay low-poly dựng từ mesh tam giác hoặc model placeholder, mặt đất grid làm mốc.
- Kết quả: bay, rẽ, ngóc/chúc mũi, tăng/giảm tốc, tự cân bằng và camera theo sau ổn định.

### Milestone M2 — Terrain + Terrain-aware Flight (P0)
- World thử nghiệm **2 km × 2 km** trước; mục tiêu mở rộng **8 km × 8 km**.
- Procedural heightfield với đồi mềm, thung lũng, vùng núi, noise nhiều tầng và masks; seed cố định để tái lập.
- Low-poly terrain mesh; query `GroundHeight(x,z)` dùng cùng dữ liệu terrain mesh, tránh mismatch render/collision.
- Collision mặt đất và terrain avoidance assist: cảnh báo/điều chỉnh nhẹ, không tự động ép bay theo địa hình.
- Camera tránh xuyên địa hình ở mức cơ bản.
- Kết quả: bay sát đồi và thung lũng, nhìn thấy thay đổi độ cao, có va chạm rõ ràng.

### Milestone M3 — World Rendering (P1)
- `SkySystem`: sky gradient/sky dome, directional light, distance fog.
- `BiomeSystem`: grassland, flower fields hồng/tím, highlands, mountain, valley; màu vật liệu/vertex color nhất quán.
- Terrain chunk (đề xuất **500 m × 500 m**), frustum/distance culling, LOD 2–3 mức; đảm bảo đường nối chunk không nứt (skirts hoặc shared edges).
- Distant mountain silhouettes/proxy; không bắt buộc streaming phức tạp ở 2x2 km.
- Mục tiêu: thế giới đẹp và đủ xa để cảm nhận tốc độ, ưu tiên FPS ổn định.

### Milestone M4 — Environment Art + Flight Polish (P2)
- Cỏ/hoa low-poly bằng batch mesh hoặc billboard; hạn chế draw call, không dùng mỗi cọng cỏ một draw call.
- Mây low-poly hoặc billboard, thay đổi độ cao/độ phủ, ưu tiên render đơn giản.
- Chase camera trễ nhẹ khi rẽ, nghiêng camera có giới hạn, FOV 65–80° theo speed, camera collision.
- Speed lines tùy chọn khi boost; tránh motion blur hậu xử lý ở giai đoạn đầu.
- Audio engine/wind và HUD tốc độ/độ cao/clearance.

### Milestone M5 — Combat + AI (P2)
- Súng máy (projectile hoặc raycast), cooldown, damage, HP, hit feedback, explosion.
- Lock-on theo góc/tầm nhìn/khoảng cách, tên lửa homing có giới hạn tốc độ quay, trail.
- Enemy AI FSM: `PATROL -> CHASE -> ATTACK`, nhánh `EVADE`, `RETREAT`; steering với tốc độ quay giới hạn, không xoay tức thời.
- 3–5 máy bay địch trong prototype; radar/HUD mục tiêu; nhiệm vụ tiêu diệt đơn giản.
- Không cần ML/LLM cho AI không chiến.

## 3. Kiến trúc module

```text
SkyCombat/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── FLIGHT_TUNING.md
│   └── WORLD_DESIGN.md
├── assets/
│   ├── models/
│   ├── textures/
│   ├── effects/
│   └── sounds/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Game.h/.cpp
│   │   ├── Time.h/.cpp
│   │   ├── Input.h/.cpp
│   │   ├── Transform.h
│   │   └── Math.h
│   ├── aircraft/
│   │   ├── Aircraft.h/.cpp
│   │   └── FlightController.h/.cpp
│   ├── camera/
│   │   └── ChaseCamera.h/.cpp
│   ├── world/
│   │   ├── WorldManager.h/.cpp
│   │   ├── HeightProvider.h
│   │   ├── TerrainSystem.h/.cpp
│   │   ├── WorldCollision.h/.cpp
│   │   ├── BiomeSystem.h/.cpp
│   │   ├── SkySystem.h/.cpp
│   │   ├── CloudSystem.h/.cpp
│   │   ├── VegetationSystem.h/.cpp
│   │   └── WorldConfig.h
│   ├── rendering/
│   │   ├── AircraftRenderer.h/.cpp
│   │   └── TerrainRenderer.h/.cpp
│   ├── combat/
│   │   ├── WeaponSystem.h/.cpp
│   │   ├── Bullet.h/.cpp
│   │   ├── Missile.h/.cpp
│   │   └── TargetingSystem.h/.cpp
│   ├── ai/
│   │   ├── EnemyAI.h/.cpp
│   │   └── SteeringBehavior.h/.cpp
│   └── ui/
│       ├── HUD.h/.cpp
│       └── Radar.h/.cpp
└── tests/
    ├── MathTests.cpp
    ├── TerrainTests.cpp
    └── FlightTests.cpp
```

**Không tạo toàn bộ file rỗng ngay từ đầu.** Tạo dần theo milestone; module nào chưa triển khai thì ghi trong roadmap, không tạo dependency chưa tồn tại.

## 4. Ranh giới trách nhiệm và luồng dữ liệu

- `Game`: quản lý lifecycle, fixed update, render, input, scene.
- `Input`: map keyboard/gamepad sang `FlightInput {pitch, turn, throttle, boost}`; không để `Aircraft` gọi trực tiếp `CheckHitKey`.
- `FlightController`: nhận input, `dt`, state và `IHeightProvider` (nếu cần), cập nhật state; **không gọi hàm render DXLib**.
- `Transform`: biểu diễn orientation ổn định, hàm forward/right/up, normalize.
- `WorldManager`: sở hữu terrain/biome/sky và cung cấp query world.
- `IHeightProvider`: `float GroundHeight(float x, float z) const` và thông tin bounds khi cần.
- `TerrainSystem`: height data + mesh generation; renderer dùng mesh nhưng không quyết định độ cao gameplay.
- `WorldCollision`: collision aircraft-terrain, clearance, camera obstruction.
- `ChaseCamera`: nhận aircraft transform, speed, `dt`, world query; tính camera pose/FOV; render layer áp vào DXLib.
- `AircraftRenderer`/`TerrainRenderer`: nhận dữ liệu immutable, gọi DXLib.
- `Combat` và `AI`: bổ sung sau, dùng transform/physics query thay vì phụ thuộc renderer.

### Thứ tự frame

1. Poll OS/messages và input.
2. Tích lũy elapsed time bằng high-resolution clock, clamp frame delta tránh spiral of death.
3. Trong fixed-step 1/60 s: update input state -> aircraft steering/speed -> transform -> terrain collision/avoidance -> AI/combat (khi có).
4. Tính render interpolation alpha cho transform.
5. Update camera từ pose được nội suy với smoothing theo render delta.
6. Clear -> render sky -> terrain -> aircraft -> effects -> HUD -> flip.
7. Không thay đổi gameplay state trong render.

## 5. Flight Model — Assisted Arcade

### Điều khiển

| Phím | Tác dụng |
|---|---|
| W / S | Pitch up / down |
| A / D | Turn left / right (yaw + banking tự động) |
| Shift | Tăng throttle / boost |
| Ctrl | Giảm throttle |
| R | Reset aircraft |
| Esc | Thoát |
| Space (M5) | Machine gun |
| F (M5) | Missile |
| Tab (M5) | Đổi target |

### Tham số ban đầu (tuning, không phải giá trị cố định)

| Parameter | Initial value |
|---|---:|
| Min speed | 25 m/s |
| Cruise speed | 70 m/s |
| Max speed | 140 m/s |
| Pitch rate | 65 deg/s |
| Yaw rate | 55 deg/s |
| Max visual bank | 50 deg |
| Auto-level response | 3.5 s^-1 (exponential) |
| Safe ground clearance | 8 m |
| Terrain warning range | 25 m |
| Camera FOV | 65–80 deg |

### Quy tắc

- Tốc độ hướng tiến cập nhật theo acceleration/deceleration và clamp; `position += forward * speed * dt`.
- Input A/D điều khiển yaw có tốc độ giới hạn; visual bank mượt tới target `-turnInput * maxBank` (kiểm tra dấu theo hệ trục đã chọn).
- Khi không rẽ, visual bank trở về 0 theo `alpha = 1 - exp(-response * dt)`; không dùng hệ số lerp cố định mỗi frame.
- Nếu chọn orientation gồm quaternion + visual bank riêng, phải đảm bảo mesh banking không làm sai hướng bay/camera. Nếu bank ảnh hưởng physical orientation, phải tính lại yaw/pitch theo local axes có kiểm soát.
- Pitch độc lập; không tự kéo mũi về 0 trừ khi thiết kế có pitch assist rõ ràng.
- Tại clearance > 25 m: không can thiệp.
- Tại 8–25 m **và đang lao xuống**: cộng trợ lực pitch-up giới hạn theo tốc độ, hướng và độ dốc địa hình (không teleport).
- Tại clearance < 8 m: cảnh báo; vẫn có thể va chạm, không bảo đảm tự cứu.
- Collision ground: xử lý nhất quán (ví dụ respawn hoặc crash state), tránh chui xuống mesh.
- Hỗ trợ world boundary (soft turn hoặc reset) để máy bay không bay ra ngoài 2x2 km.

## 6. Terrain và World Map

### Layout 2x2 km

- Trung tâm: grassland rộng, vùng tập bay/dogfight.
- Bắc: núi cao và cao nguyên.
- Nam: thung lũng uốn lượn, đường bay sát đất.
- Tây: cánh đồng hoa tím.
- Đông: cánh đồng hoa hồng.
- Spawn tại vùng trống, có độ cao đủ để không va chạm ngay.
- Bản đồ 8x8 km là mục tiêu dài hạn; không tối ưu sớm bằng streaming nếu chưa có vấn đề hiệu năng.

### Height function

`H(x,z) = Hills(x,z) + MountainMask(x,z)*Mountains(x,z) + Detail(x,z) - ValleyMask(x,z)*ValleyDepth(x,z)`

- Noise deterministic có seed, tần số thấp cho đồi, tần số cao cho chi tiết; giới hạn độ dốc tại đường bay chính.
- Mesh terrain lấy mẫu đúng height function; query collision dùng cùng interpolation (bilinear hoặc triangle-consistent), không sử dụng hàm khác gây lệch mặt đất.
- Normals cho flat shading hoặc per-face normals để giữ low-poly look; ánh sáng mềm, tránh noise hình ảnh quá dày.
- Biome phân theo vị trí, độ cao và độ dốc; blend màu chuyển vùng, tránh ranh giới gắt.
- Bảo đảm terrain nằm trong bounds và hành vi ngoài bounds được định nghĩa.

### Chunk/LOD sau prototype

- Chunk mục tiêu 500x500 m, 2x2 km = 4x4 chunk; 8x8 km = 16x16 chunk.
- Gần: high LOD; trung: medium; xa: low LOD; rất xa: mountain proxy/silhouette.
- Culling theo camera frustum/distance; tránh pop bằng hysteresis hoặc crossfade nếu khả thi.
- Chống nứt mép LOD bằng skirts hoặc shared boundary vertices.
- Vegetation batching; giới hạn draw calls; không tạo texture/shader mới mỗi frame.

## 7. Chase Camera và cảm giác tốc độ

- Third-person camera phía sau/trên máy bay, nhìn về phía trước máy bay; target offset theo speed.
- Smoothing `1-exp(-lambda*dt)` cho position/target, tách tham số follow và look-at.
- FOV biến thiên mượt 65–80 độ theo speed; tránh thay đổi quá nhanh gây khó chịu.
- Roll camera chỉ là phần nhỏ của bank máy bay, có giới hạn để horizon dễ đọc.
- Camera terrain collision: sample ground/cast đoạn camera-target nếu cần; nâng camera khỏi terrain với margin; không giật khi đi qua đỉnh đồi.
- Speed lines chỉ xuất hiện trên ngưỡng tốc độ, hiệu ứng nhẹ và có toggle; chưa cần post-process motion blur.
- HUD: speed m/s hoặc km/h (ghi đơn vị), altitude, clearance, FPS, trạng thái assist.

## 8. Rendering bằng DXLib

- Dùng DXLib cho window, graphics, 3D primitives/model, input, audio. Xác minh chữ ký hàm và linker libs từ SDK cài trên máy.
- Đặt state render rõ ràng: depth test/write, culling, lighting, fog và alpha blending phù hợp từng pass.
- Máy bay placeholder low-poly có màu thân/cánh tương phản; hướng mũi máy bay khớp `Transform.forward`.
- Sky gradient/sky dome và fog giúp khoảng cách nhìn tự nhiên; tránh dựng hàng triệu polygon để có cảnh xa.
- Với vegetation/clouds, ưu tiên mesh batch hoặc billboard; chỉ thêm custom shaders khi DXLib API và pipeline được kiểm chứng.
- Tránh phụ thuộc Effekseer ở M1–M3; có thể tích hợp cho explosion/trails ở M5 nếu cần.

## 9. Build & môi trường

- Visual Studio 2022 Community, workload **Desktop development with C++**, Windows SDK, CMake, DXLib SDK Visual C++ x64.
- `CMakeLists.txt` phải nhận `DXLIB_ROOT` qua `-D` hoặc environment, tìm đúng include và `.lib` theo SDK thực tế; không hardcode đường dẫn máy developer.
- Dùng C++20, target Windows x64, Unicode/encoding phù hợp, copy assets tới runtime output nếu có.
- Ví dụ (đường dẫn chỉ minh họa, điều chỉnh theo SDK):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDXLIB_ROOT="C:/SDK/DxLib/プロジェクトに追加すべきファイル_VC用"
cmake --build build --config Debug
.\build\Debug\SkyCombat.exe
```

- Kiểm tra thư viện DXLib phụ thuộc và runtime file theo gói SDK thực tế; không cam kết câu lệnh trên chạy nguyên xi trên mọi bản SDK.

## 10. Kiểm thử và tiêu chí nghiệm thu

### M1
- Mở window 1280x720; thoát Esc; không treo khi mất focus.
- 60 Hz fixed update; camera/điều khiển không phụ thuộc FPS; test 30/60/144 FPS.
- W/S pitch, A/D turn+bank, thả A/D auto-level, Shift/Ctrl thay đổi speed có clamp.
- Reset R khôi phục state, camera không bị giật kéo dài.
- Máy bay render đúng hướng mũi, không đảo trục hoặc bị lật bất ngờ.

### M2
- Terrain 2x2 km hiển thị liên tục, không có lỗ/méo mesh.
- `GroundHeight` trùng mặt mesh tại nhiều điểm mẫu; collision không xuyên đất.
- Bay thấp có cảnh báo/assist, không ép bám terrain; va chạm có kết quả rõ ràng.
- Camera không thường xuyên chui vào đất.

### M3–M4
- Chuyển biome tự nhiên; núi xa, fog, mây và cánh đồng hoa có chiều sâu.
- Bay tốc độ cao không thấy chunk seam rõ rệt; giảm pop LOD.
- Đo FPS, frame time, triangle/draw-call count nếu có công cụ; ưu tiên ổn định trên cấu hình máy người dùng thay vì tự đặt KPI vô căn cứ.

### M5
- Súng/đạn và tên lửa có cooldown, hit detection, damage.
- Lock-on chỉ hợp lệ trong tầm/góc quy định.
- 3–5 AI chuyển state đúng, không quay tức thời, có thể tiêu diệt.

## 11. Rủi ro và quyết định kỹ thuật

- **DXLib API / CMake linking:** phụ thuộc bản SDK và thư viện, cần xác minh trên Windows.
- **Quaternion vs visual bank:** tránh nhầm visual roll với hướng bay thực; viết test rotation.
- **Terrain mesh vs collision:** phải dùng cùng dữ liệu mẫu/interpolation.
- **High speed tunneling:** collision nên dùng swept checks hoặc substeps cho vật thể tốc độ cao; nhất là khi thêm đạn/tên lửa.
- **Camera smoothing:** clamp `dt`, reset camera state khi respawn/teleport.
- **Performance:** batching/LOD chỉ bổ sung khi có profiling; prototype nên giữ cấu trúc đơn giản.
- **Bản quyền:** tham chiếu video về cảm giác, nhịp độ, palette; không tái tạo asset/game khác nguyên bản.

## 12. Lộ trình ước lượng

| Tuần | Nội dung | Kết quả |
|---|---|---|
| 1 | Game loop, input, Transform, Aircraft, Chase Camera | Bay tự do assisted arcade |
| 2 | Procedural terrain, height query, collision/avoidance | Bay sát đồi/thung lũng |
| 3 | Sky/fog, biomes, màu terrain | World có chiều sâu |
| 4 | Chunk LOD/culling, núi xa | World render ổn định |
| 5 | Cỏ/hoa/mây, camera polish, speed effects | Cảm giác bay giống tham chiếu |
| 6+ | Combat, enemy FSM, targeting, missiles, HUD | Prototype dogfight |

Ước lượng linh hoạt, không phải cam kết thời hạn.

## 13. Cách agent phải bắt đầu ngay

**Không triển khai toàn bộ 6 tuần trong một lượt.** Trước tiên:

1. Kiểm tra repository hiện tại, `CMakeLists.txt`, DXLib SDK path, các class đã có. Nếu có project v0.1/v0.2, **tái sử dụng và sửa**, không ghi đè vô điều kiện.
2. Trình bày danh sách file cần tạo/sửa cho milestone gần nhất, cùng dependency và tiêu chí kiểm thử.
3. Triển khai **M1 nếu chưa chạy được**, nếu M1 ổn thì triển khai **M2 terrain + terrain-aware flight**.
4. Viết code thật cho toàn bộ các file được nhắc tới trong milestone; tránh pseudocode hoặc placeholder không chạy.
5. Build/test trên môi trường có Windows+DXLib nếu khả thi. Nếu không thể build, nêu rõ lý do và cách user kiểm chứng; không tuyên bố thành công.
6. Tóm tắt kết quả và đề xuất một bước tiếp theo nhỏ, có thể kiểm thử.

## 14. Prompt ngắn để giao việc từng lượt

> Hãy đọc toàn bộ `SkyCombat_3D_Master_Prompt.md` và kiểm tra repository SkyCombat hiện tại. Tuân thủ C++20 + DXLib, Assisted Arcade, low-poly, World Map đồng cỏ/hoa/núi/thung lũng và camera tốc độ cao. Triển khai milestone nhỏ nhất chưa hoàn thành, ưu tiên M1 -> M2 -> M3 -> M4 -> M5. Cung cấp code thật, build/test khi có môi trường, báo cáo file thay đổi và lỗi còn lại. Không tự chuyển engine, không khẳng định build thành công nếu chưa xác minh.
