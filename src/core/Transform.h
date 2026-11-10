#pragma once
#include "Math.h"
namespace sky {
// Metres; +X right, +Y up, +Z forward. Orientation excludes visual bank.
struct Transform {
    Vec3 position{};
    Quat orientation{};
    Vec3 forward() const { return orientation.rotate({0,0,1}); }
    Vec3 right() const { return orientation.rotate({1,0,0}); }
    Vec3 up() const { return orientation.rotate({0,1,0}); }
};
inline Transform interpolate(const Transform& a,const Transform& b,double alpha) {
    return {lerp(a.position,b.position,alpha),slerp(a.orientation,b.orientation,alpha)};
}
}
