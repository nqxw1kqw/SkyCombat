#!/bin/bash
# SkyCombat — MinGW (GCC) + DxLib GCC build, mirrors the CMake target's source list.
set -e
cd "$(dirname "$0")"
DXINC="$HOME/dxlib-inc"
CXX=x86_64-w64-mingw32-g++
SRCS="src/main.cpp src/core/Game.cpp src/core/Input.cpp src/core/MouseSteering.cpp src/core/Time.cpp
src/aircraft/Aircraft.cpp src/aircraft/FlightController.cpp
src/camera/ChaseCamera.cpp src/rendering/AircraftRenderer.cpp src/rendering/ObjMesh.cpp
src/world/TerrainSystem.cpp src/world/WorldCollision.cpp
src/world/BiomeSystem.cpp src/world/SkySystem.cpp
src/world/TerrainChunk.cpp
src/world/VegetationSystem.cpp src/world/CloudSystem.cpp
src/effects/SpeedEffects.cpp src/audio/AudioModel.cpp
src/combat/WeaponSystem.cpp src/combat/TargetingSystem.cpp
src/combat/Missile.cpp src/combat/MissionSystem.cpp
src/ai/SteeringBehavior.cpp src/ai/EnemyAI.cpp
src/ui/HUD.cpp src/ui/Radar.cpp src/ui/FlightReticle.cpp
src/rendering/TerrainRenderer.cpp"
mkdir -p ../build
$CXX -std=c++20 -O2 -DUNICODE -D_UNICODE -DNOMINMAX \
  -Isrc -I"$DXINC" \
  -o ../build/SkyCombat.exe $SRCS \
  -L"$DXINC" -Wl,--start-group \
  -lDxLibW -lDxUseCLibW -lDxDrawFunc \
  -ljpeg -lpng -lzlib -ltiff -ltheora_static -lvorbis_static -lvorbisfile_static -logg_static \
  -lbulletdynamics -lbulletcollision -lbulletmath \
  -lopusfile -lopus -lcelt -lsilk_common \
  -Wl,--end-group \
  -static-libgcc -static-libstdc++ -mwindows \
  -lgdi32 -lwinmm -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 -lshell32 -lversion -lws2_32 -lsetupapi -ldxguid -ldinput8 -lxinput
echo "BUILD OK: ../build/SkyCombat.exe"
