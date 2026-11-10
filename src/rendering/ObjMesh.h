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
    // Parses without normalising, keeping the authored coordinates. Used
    // for attachments (plumes, airflow shells) that share the parent
    // model's local frame and must receive the same transform as it.
    bool loadRaw(const std::string& objPath);
    void bounds(Vec3& mn, Vec3& mx) const;
    void applyTransform(const Vec3& center, double scale);
    bool empty() const { return triangles_.empty(); }
    const std::vector<Triangle>& triangles() const { return triangles_; }

private:
    std::vector<Triangle> triangles_;
};

} // namespace sky
