#pragma once
#include "core/Math.h"
#include <cstdint>

namespace sky {

struct SkyColors {
    Vec3 zenith{0.28, 0.58, 0.90};    // Bright azure sky
    Vec3 horizon{0.70, 0.84, 0.95};   // Soft pale horizon blue
    Vec3 sunDirection{0.4, 0.8, 0.3}; // Downward/forward sun vector
    Vec3 sunColor{1.0, 0.98, 0.90};   // Warm sunlight
};

struct FogConfig {
    Vec3 color{0.70, 0.84, 0.95};     // Matches horizon for seamless sky fade
    double nearDistance{500.0};
    double farDistance{3500.0};
    bool enabled{true};
};

class SkySystem {
public:
    SkySystem();

    const SkyColors& colors() const { return colors_; }
    void setColors(const SkyColors& colors) { colors_ = colors; }

    const FogConfig& fog() const { return fog_; }
    void setFog(const FogConfig& fog) { fog_ = fog; }

    // Evaluates sky color given view direction
    Vec3 evaluateSkyColor(const Vec3& viewDir) const;

    // Evaluates fog factor [0, 1] for a given distance
    double evaluateFogFactor(double distance) const;

    // 0x00RRGGBB packed color helpers for DXLib
    uint32_t packedZenithColor() const;
    uint32_t packedHorizonColor() const;
    uint32_t packedFogColor() const;

private:
    SkyColors colors_;
    FogConfig fog_;
};

} // namespace sky
