#pragma once
#include "TerrainMesh.h"
#include "WorldConfig.h"
#include <vector>
#include <cstdint>
#include <cstddef>

namespace sky {

struct CloudCluster {
    Vec3 position{0, 280, 0};
    Vec3 size{80, 20, 100};
    int boxCount{3};
};

struct ClusterVertexRange {
    size_t startVertex{0};
    size_t vertexCount{0};
};

class CloudSystem {
public:
    explicit CloudSystem(uint32_t seed = WorldConfig::fixedSeed);

    void initialize();

    // Drifts cloud positions with time delta, wrapping seamlessly without per-frame allocations
    void update(double dt);

    const TerrainMeshData& mesh() const { return mesh_; }
    size_t cloudCount() const { return clusters_.size(); }
    const std::vector<CloudCluster>& clusters() const { return clusters_; }

    double minAltitude() const { return minAltitude_; }
    double maxAltitude() const { return maxAltitude_; }
    double driftSpeed() const { return driftSpeed_; }
    uint32_t seed() const { return seed_; }

private:
    void rebuildMesh();

    uint32_t seed_{WorldConfig::fixedSeed};
    std::vector<CloudCluster> clusters_;
    std::vector<ClusterVertexRange> clusterRanges_;
    TerrainMeshData mesh_;

    double minAltitude_{WorldConfig::cloudMinAltitude};
    double maxAltitude_{WorldConfig::cloudMaxAltitude};
    double driftSpeed_{WorldConfig::cloudDriftSpeed};
};

} // namespace sky
