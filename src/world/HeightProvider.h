#pragma once
#include "WorldConfig.h"

namespace sky {
class IHeightProvider {
public:
    virtual ~IHeightProvider() = default;

    // Ground height at (x,z) matching the rendered mesh exactly
    virtual double groundHeight(double x, double z) const = 0;

    // Spec aliases
    double GroundHeight(double x, double z) const { return groundHeight(x, z); }
    float GroundHeight(float x, float z) const { return float(groundHeight(x, z)); }

    // World bounds check
    virtual bool inBounds(double x, double z) const {
        return x >= WorldConfig::minX && x <= WorldConfig::maxX &&
               z >= WorldConfig::minZ && z <= WorldConfig::maxZ;
    }
};
}
