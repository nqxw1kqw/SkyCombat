#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
sources=(src/core/Input.cpp src/core/Time.cpp src/aircraft/Aircraft.cpp src/aircraft/FlightController.cpp src/camera/ChaseCamera.cpp)
for suite in MathTests FlightTests; do
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Isrc "tests/$suite.cpp" "${sources[@]}" -o "$build_dir/$suite"
    printf '%s: ' "$suite"
    "$build_dir/$suite"
done
