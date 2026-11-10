#include "AircraftRenderer.h"
#include <DxLib.h>
#include <array>
namespace sky {
void AircraftRenderer::draw(const Aircraft& a) const {
    // Local nose +Z; roll is purely visual and cannot steer flight or camera.
    const Quat rotation=a.transform.orientation*Quat::axisAngle({0,0,1},a.visualBank);
    const std::array<Vec3,10> points{{{0,0,7},{-1,0,-4},{1,0,-4},{0,1.3,-1},
        {0,-0.8,-1},{-6,0,-2},{6,0,-2},{-2.8,0,-4.5},{2.8,0,-4.5},{0,2.8,-4}}};
    std::array<VECTOR,10> v{};
    for(size_t i=0;i<v.size();++i) { auto p=a.transform.position+rotation.rotate(points[i]); v[i]=VGet(float(p.x),float(p.y),float(p.z)); }
    struct Face { int a,b,c; unsigned int color; };
    const auto body=GetColor(225,232,239),shade=GetColor(117,143,166),wing=GetColor(239,163,65);
    const Face faces[]={{0,1,3,body},{0,3,2,body},{0,4,1,shade},{0,2,4,shade},
        {1,2,3,shade},{1,4,2,shade},{0,5,1,wing},{0,2,6,wing},
        {1,7,3,wing},{2,3,8,wing},{1,9,2,body}};
    for(auto f:faces) DrawTriangle3D(v[f.a],v[f.b],v[f.c],f.color,TRUE);
}
}
