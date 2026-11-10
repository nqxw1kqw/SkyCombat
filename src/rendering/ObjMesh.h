#pragma once
#include "core/Math.h"
#include <string>
#include <vector>

namespace sky {

// Minimal Wavefront OBJ + MTL loader for flat-coloured low-poly models.
// Parses v / f / usemtl / mtllib with Kd diffuse colours, fan-triangulates
// polygon faces, then normalises the mesh: bounding box centred on the
// origin and the longest axis scaled to a target length in metres.
// Rendering stays with the caller (DrawTriangle3D); colours are 0xRRGGBB.
class ObjMesh {
public:
    struct Triangle {
        Vec3 p[3];
        unsigned int color{0xFFFFFF};
    };

    bool load(const std::string& objPath, double targetLength);
    bool empty() const { return triangles_.empty(); }
    const std::vector<Triangle>& triangles() const { return triangles_; }

private:
    std::vector<Triangle> triangles_;
};

} // namespace sky
