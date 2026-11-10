#pragma once
#include <algorithm>
#include <cmath>
namespace sky {
constexpr double pi = 3.14159265358979323846;
constexpr double radians(double degrees) { return degrees*pi/180.0; }
struct Vec3 {
    double x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
};
inline double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline double length(Vec3 v) { return std::sqrt(dot(v,v)); }
inline Vec3 normalized(Vec3 v) { double n=length(v); return n>1e-12?v*(1/n):Vec3{}; }
inline Vec3 lerp(Vec3 a, Vec3 b, double t) { return a+(b-a)*t; }
inline double response(double rate,double dt) { return -std::expm1(-rate*std::max(0.0,dt)); }
struct Quat {
    double w{1},x{},y{},z{};
    Quat operator*(Quat b) const {
        return {w*b.w-x*b.x-y*b.y-z*b.z,w*b.x+x*b.w+y*b.z-z*b.y,
                w*b.y-x*b.z+y*b.w+z*b.x,w*b.z+x*b.y-y*b.x+z*b.w};
    }
    Quat normalized() const {
        double n=std::sqrt(w*w+x*x+y*y+z*z);
        return n>1e-12?Quat{w/n,x/n,y/n,z/n}:Quat{};
    }
    static Quat axisAngle(Vec3 axis,double angle) {
        axis=sky::normalized(axis); double s=std::sin(angle/2);
        return {std::cos(angle/2),axis.x*s,axis.y*s,axis.z*s};
    }
    Vec3 rotate(Vec3 v) const {
        Quat q=normalized(); Quat r=q*Quat{0,v.x,v.y,v.z}*Quat{q.w,-q.x,-q.y,-q.z};
        return {r.x,r.y,r.z};
    }
};
inline Quat slerp(Quat a,Quat b,double t) {
    a=a.normalized(); b=b.normalized();
    double d=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
    if(d<0) { b={-b.w,-b.x,-b.y,-b.z}; d=-d; }
    double u=1-t,v=t;
    if(d<0.9995) { double theta=std::acos(std::clamp(d,-1.0,1.0));
        u=std::sin((1-t)*theta)/std::sin(theta); v=std::sin(t*theta)/std::sin(theta); }
    return Quat{u*a.w+v*b.w,u*a.x+v*b.x,u*a.y+v*b.y,u*a.z+v*b.z}.normalized();
}
}
