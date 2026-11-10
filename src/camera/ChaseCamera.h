#pragma once
#include "aircraft/Aircraft.h"

namespace sky {

class IHeightProvider;

struct CameraPose { Vec3 position{}, target{}, up{0, 1, 0}; double fovDegrees{65}; };

class ChaseCamera {
public:
    // Camera presets scale the follow distance/height (1.0 = standard
    // chase). The nose preset is posed by the game layer, not here.
    void setFollowScale(double scale) { followScale_ = scale; }

    void reset(const Aircraft& aircraft, const IHeightProvider* terrain = nullptr);
    void update(const Aircraft& aircraft, double dt, const IHeightProvider* terrain = nullptr);
    const CameraPose& pose() const { return pose_; }

    CameraPose desired(const Aircraft& aircraft) const;

private:
    void applyTerrainAvoidance(const IHeightProvider* terrain);
    CameraPose pose_;
    double followScale_{1.0};
};

}
