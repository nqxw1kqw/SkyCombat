#include "ObjMesh.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>

namespace sky {

namespace {

unsigned int packColor(double r, double g, double b) {
    auto ch = [](double v) {
        return static_cast<unsigned int>(std::clamp(int(v * 255.0 + 0.5), 0, 255));
    };
    return (ch(r) << 16) | (ch(g) << 8) | ch(b);
}

std::string dirOf(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string{} : path.substr(0, slash + 1);
}

// Parses an MTL file into material name -> packed Kd colour.
std::map<std::string, unsigned int> loadMtl(const std::string& mtlPath) {
    std::map<std::string, unsigned int> colors;
    std::ifstream in(mtlPath);
    if (!in) return colors;
    std::string line, current;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string key;
        if (!(ss >> key)) continue;
        if (key == "newmtl") {
            ss >> current;
        } else if (key == "Kd" && !current.empty()) {
            double r = 1.0, g = 1.0, b = 1.0;
            ss >> r >> g >> b;
            colors[current] = packColor(r, g, b);
        }
    }
    return colors;
}

// Face tokens come as "v", "v/vt", "v//vn" or "v/vt/vn"; OBJ indices are
// 1-based and may be negative (relative to the vertices defined so far).
int vertexIndex(const std::string& token, size_t vertexCount) {
    const std::string head = token.substr(0, token.find('/'));
    if (head.empty()) return -1;
    int idx = std::stoi(head);
    if (idx < 0) idx = int(vertexCount) + idx + 1;
    return idx - 1;
}

} // namespace

bool ObjMesh::load(const std::string& objPath, double targetLength) {
    triangles_.clear();
    std::ifstream in(objPath);
    if (!in) return false;

    const std::string baseDir = dirOf(objPath);
    std::vector<Vec3> verts;
    std::map<std::string, unsigned int> materials;
    unsigned int currentColor = 0xFFFFFF;

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string key;
        if (!(ss >> key)) continue;
        if (key == "mtllib") {
            std::string mtlName;
            ss >> mtlName;
            for (const auto& [name, col] : loadMtl(baseDir + mtlName)) materials[name] = col;
        } else if (key == "v") {
            Vec3 v{};
            ss >> v.x >> v.y >> v.z;
            verts.push_back(v);
        } else if (key == "usemtl") {
            std::string name;
            ss >> name;
            const auto it = materials.find(name);
            currentColor = it != materials.end() ? it->second : 0xFFFFFF;
        } else if (key == "f") {
            std::vector<int> face;
            std::string token;
            while (ss >> token) {
                const int idx = vertexIndex(token, verts.size());
                if (idx >= 0 && idx < int(verts.size())) face.push_back(idx);
            }
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                Triangle tri;
                tri.p[0] = verts[size_t(face[0])];
                tri.p[1] = verts[size_t(face[i])];
                tri.p[2] = verts[size_t(face[i + 1])];
                tri.color = currentColor;
                triangles_.push_back(tri);
            }
        }
    }
    if (triangles_.empty()) return false;

    // Normalise: centre the bounding box on the origin and scale the
    // longest axis to the target length so any source model lands at
    // game scale regardless of its authoring units.
    Vec3 mn = triangles_[0].p[0], mx = mn;
    for (const auto& tri : triangles_) {
        for (const auto& p : tri.p) {
            mn.x = std::min(mn.x, p.x); mn.y = std::min(mn.y, p.y); mn.z = std::min(mn.z, p.z);
            mx.x = std::max(mx.x, p.x); mx.y = std::max(mx.y, p.y); mx.z = std::max(mx.z, p.z);
        }
    }
    const Vec3 center = (mn + mx) * 0.5;
    const double extent = std::max({mx.x - mn.x, mx.y - mn.y, mx.z - mn.z});
    const double scale = extent > 1e-9 ? targetLength / extent : 1.0;
    for (auto& tri : triangles_) {
        for (auto& p : tri.p) p = (p - center) * scale;
    }
    return true;
}

} // namespace sky
