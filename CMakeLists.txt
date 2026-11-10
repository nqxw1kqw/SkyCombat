cmake_minimum_required(VERSION 3.20)
project(SkyCombat LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "SkyCombat requires Windows x64 with Visual Studio 2022. On Linux run tests/run_portable.sh.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select x64: -A x64")
endif()
set(DXLIB_ROOT "$ENV{DXLIB_ROOT}" CACHE PATH "DXLib Visual C++ SDK directory")
find_path(DXLIB_INCLUDE_DIR DxLib.h HINTS "${DXLIB_ROOT}" PATH_SUFFIXES include REQUIRED)
find_library(DXLIB_LIBRARY NAMES DxLib_x64 HINTS "${DXLIB_ROOT}" PATH_SUFFIXES lib lib/x64 REQUIRED)
get_filename_component(DXLIB_LIBRARY_DIR "${DXLIB_LIBRARY}" DIRECTORY)
# Official MSVC SDK headers select the matching DXLib/dependency libraries via
# pragma comment(lib). Keep the complete SDK .lib directory available to LINK.
add_executable(SkyCombat WIN32
    src/main.cpp src/core/Game.cpp src/core/Input.cpp src/core/Time.cpp
    src/aircraft/Aircraft.cpp src/aircraft/FlightController.cpp
    src/camera/ChaseCamera.cpp src/rendering/AircraftRenderer.cpp)
target_compile_features(SkyCombat PRIVATE cxx_std_20)
set_target_properties(SkyCombat PROPERTIES CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(SkyCombat PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(SkyCombat PRIVATE /W4 /utf-8)
target_include_directories(SkyCombat PRIVATE src "${DXLIB_INCLUDE_DIR}")
target_link_directories(SkyCombat PRIVATE "${DXLIB_LIBRARY_DIR}")
# DxLib.h autolinks the debug/release variant and bundled codec dependencies.
# Do not also force the release library into Debug builds.
