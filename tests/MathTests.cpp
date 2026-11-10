#include "Test.h"
#include "core/Transform.h"
using namespace sky;
int main() {
    Tests t; Transform a;
    t.check(near(a.forward().z,1)&&near(a.right().x,1)&&near(a.up().y,1),"identity axes");
    a.orientation=Quat::axisAngle({0,1,0},radians(90));
    t.check(near(a.forward().x,1),"positive yaw turns right");
    a.orientation=Quat::axisAngle({1,0,0},radians(-90));
    t.check(near(a.forward().y,1),"negative local X pitch raises nose");
    for(int i=0;i<100000;++i) a.orientation=(a.orientation*Quat::axisAngle({1,0,0},0.001)).normalized();
    t.check(near(length(a.forward()),1)&&near(dot(a.forward(),a.up()),0),"long rotation orthonormal");
    Transform b{{10,20,30},Quat::axisAngle({0,1,0},radians(90))};
    auto mid=interpolate(Transform{},b,0.5);
    t.check(near(mid.position.x,5)&&near(mid.forward().x,std::sqrt(0.5)),"transform interpolation");
    Quat q=b.orientation;
    t.check(near(slerp(q,{-q.w,-q.x,-q.y,-q.z},0.5).rotate({0,0,1}).x,1),"antipodal quaternion shortest path");
    t.check(near(response(3.5,1),1-std::exp(-3.5)),"exponential response");
    t.check(near(length(normalized({})),0),"zero normalization safe");
    return t.finish();
}
