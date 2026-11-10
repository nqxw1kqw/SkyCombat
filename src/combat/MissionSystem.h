#pragma once
#include "aircraft/Aircraft.h"
#include "ai/EnemyAI.h"
#include <vector>
#include <cstddef>

namespace sky {

enum class MissionStatus {
    Active,
    Cleared,
    Failed
};

class MissionSystem {
public:
    explicit MissionSystem(size_t totalEnemies = 4);

    void reset(size_t totalEnemies = 4);

    void update(double dt, const Aircraft& player, const std::vector<EnemyAI>& enemies);

    MissionStatus status() const { return status_; }
    size_t totalEnemies() const { return totalEnemies_; }
    size_t enemiesRemaining() const { return enemiesRemaining_; }
    double missionTime() const { return missionTime_; }

    bool isCleared() const { return status_ == MissionStatus::Cleared; }
    bool isFailed() const { return status_ == MissionStatus::Failed; }
    bool isActive() const { return status_ == MissionStatus::Active; }

private:
    MissionStatus status_{MissionStatus::Active};
    size_t totalEnemies_{4};
    size_t enemiesRemaining_{4};
    double missionTime_{0.0};
};

} // namespace sky
