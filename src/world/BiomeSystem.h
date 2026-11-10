#pragma once
#include "core/Math.h"
#include <cstdint>
#include <string_view>

namespace sky {

enum class BiomeType : uint8_t {
    Grassland,
    PurpleFlowerField,
    PinkFlowerField,
    Highland,
    MountainRock,
    ValleyFloor
};

struct BiomeWeights {
    double grassland{0};
    double purpleFlower{0};
    double pinkFlower{0};
    double highland{0};
    double mountainRock{0};
    double valleyFloor{0};
};

class BiomeSystem {
public:
    BiomeSystem() = default;

    // Computes normalized biome weights at any terrain location
    BiomeWeights evaluateWeights(double x, double z, double y, const Vec3& normal) const;

    // Classifies the predominant biome at a location
    BiomeType classifyBiome(double x, double z, double y, const Vec3& normal) const;

    // Computes smoothly blended RGB color (0.0 to 1.0)
    Vec3 evaluateColor(double x, double z, double y, const Vec3& normal) const;

    // DXLib-compatible 0x00RRGGBB packed color with optional directional lighting
    uint32_t evaluatePackedColor(double x, double z, double y, const Vec3& normal,
                                const Vec3& lightDir = {0.4, 0.8, 0.3}) const;

    static std::string_view biomeName(BiomeType type);

    // Reference palette colors (linear RGB [0, 1])
    static constexpr Vec3 colorGrassland{0.55, 0.77, 0.45};     // #8cc473 - fresh grassland green
    static constexpr Vec3 colorPurpleFlower{0.74, 0.58, 0.89};  // #bc94e2 - lavender blossom
    static constexpr Vec3 colorPinkFlower{0.93, 0.71, 0.79};    // #edb5c9 - rose pink blossom
    static constexpr Vec3 colorHighland{0.58, 0.69, 0.48};      // #94b07a - upland olive
    static constexpr Vec3 colorMountainRock{0.58, 0.61, 0.56};  // #949c8f - mountain slate
    static constexpr Vec3 colorPeakSnow{0.82, 0.85, 0.88};      // #d1d9e0 - peak granite/snow
    static constexpr Vec3 colorValleyFloor{0.78, 0.68, 0.50};   // #c7ad80 - canyon sand/ochre
};

} // namespace sky
