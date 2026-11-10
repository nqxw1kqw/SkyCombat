#pragma once
#include "aircraft/Aircraft.h"
namespace sky {
struct CameraPose { Vec3 position{},target{},up{0,1,0}; double fovDegrees{65}; };
class ChaseCamera {
public:
    void reset(const Aircraft& aircraft);
    void update(const Aircraft& aircraft,double dt);
    const CameraPose& pose() const { return pose_; }
private:
    CameraPose desired(const Aircraft& aircraft) const;
    CameraPose pose_;
};
}
