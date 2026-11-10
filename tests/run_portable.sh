#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
sources=(
    src/core/Input.cpp
    src/core/MouseSteering.cpp
    src/core/Time.cpp
    src/aircraft/Aircraft.cpp
    src/aircraft/FlightController.cpp
    src/camera/ChaseCamera.cpp
    src/world/TerrainSystem.cpp
    src/world/WorldCollision.cpp
    src/world/BiomeSystem.cpp
    src/world/SkySystem.cpp
    src/world/TerrainChunk.cpp
    src/world/VegetationSystem.cpp
    src/world/CloudSystem.cpp
    src/effects/SpeedEffects.cpp
    src/audio/AudioModel.cpp
    src/combat/WeaponSystem.cpp
    src/combat/TargetingSystem.cpp
    src/combat/Missile.cpp
    src/combat/MissionSystem.cpp
    src/ai/SteeringBehavior.cpp
    src/ai/EnemyAI.cpp
    src/ui/HUD.cpp
    src/ui/Radar.cpp
)
objs=()
for src in "${sources[@]}"; do
    obj="$build_dir/$(echo "$src" | tr '/' '_').o"
    g++ -std=c++20 -DSKY_PORTABLE -Wall -Wextra -Wpedantic -Werror -O2 -Isrc -c "$src" -o "$obj"
    objs+=("$obj")
done
for suite in MathTests FlightTests TerrainTests WorldRenderTests PolishTests CombatTests MouseSteeringTests; do
    g++ -std=c++20 -DSKY_PORTABLE -Wall -Wextra -Wpedantic -Werror -O2 -Isrc "tests/$suite.cpp" "${objs[@]}" -o "$build_dir/$suite"
    printf '%s: ' "$suite"
    "$build_dir/$suite"
done
