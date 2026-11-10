#include "SkySystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

namespace {

inline uint32_t packRGB(const Vec3& c) {
    int r = std::clamp(int(c.x * 255.0 + 0.5), 0, 255);
    int g = std::clamp(int(c.y * 255.0 + 0.5), 0, 255);
    int b = std::clamp(int(c.z * 255.0 + 0.5), 0, 255);
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

} // namespace

SkySystem::SkySystem() {
    colors_.sunDirection = normalized(colors_.sunDirection);
}

Vec3 SkySystem::evaluateSkyColor(const Vec3& viewDir) const {
    Vec3 dir = normalized(viewDir);
    // Smooth gradient between horizon and zenith based on positive vertical angle
    double t = std::clamp(dir.y, 0.0, 1.0);
    // Non-linear power gives richer azure dome with broad horizon haze
    t = std::pow(t, 0.6);
    return lerp(colors_.horizon, colors_.zenith, t);
}

double SkySystem::evaluateFogFactor(double distance) const {
    if (!fog_.enabled || distance <= fog_.nearDistance) return 0.0;
    if (distance >= fog_.farDistance) return 1.0;
    return (distance - fog_.nearDistance) / (fog_.farDistance - fog_.nearDistance);
}

uint32_t SkySystem::packedZenithColor() const {
    return packRGB(colors_.zenith);
}

uint32_t SkySystem::packedHorizonColor() const {
    return packRGB(colors_.horizon);
}

uint32_t SkySystem::packedFogColor() const {
    return packRGB(fog_.color);
}

} // namespace sky
