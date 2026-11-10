#pragma once
#include "HeightProvider.h"
#include <vector>
#include <cstdint>

namespace sky {

struct TerrainVertex {
    Vec3 position;
    Vec3 normal{0, 1, 0};
    uint32_t color{0};
};

struct TerrainTriangle {
    uint32_t i0{}, i1{}, i2{};
    Vec3 normal{0, 1, 0};
    uint32_t color{0};
};

struct TerrainMeshData {
    std::vector<TerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<TerrainTriangle> triangles;
};

class TerrainSystem : public IHeightProvider {
public:
    explicit TerrainSystem(uint32_t seed = WorldConfig::fixedSeed);

    // Analytic height function: H(x,z) = Hills + MountainMask*Mountains + Detail - ValleyMask*ValleyDepth
    static double evaluateAnalyticHeight(double x, double z, uint32_t seed = WorldConfig::fixedSeed);

    // Component functions for inspection, tests and design tuning
    static double evaluateHills(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateMountainMask(double x, double z);
    static double evaluateMountains(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateDetail(double x, double z, uint32_t seed = WorldConfig::fixedSeed);
    static double evaluateValleyMask(double x, double z);
    static double evaluateValleyDepth(double x, double z, uint32_t seed = WorldConfig::fixedSeed);

    // Initializes grid data and builds low-poly mesh
    void initialize();

    // IHeightProvider: triangle-consistent ground height matching mesh exactly
    double groundHeight(double x, double z) const override;

    uint32_t seed() const { return seed_; }
    bool initialized() const { return initialized_; }
    const TerrainMeshData& mesh() const { return mesh_; }
    const std::vector<double>& heights() const { return heights_; }

private:
    uint32_t seed_{WorldConfig::fixedSeed};
    std::vector<double> heights_;
    TerrainMeshData mesh_;
    bool initialized_{false};
};

}
