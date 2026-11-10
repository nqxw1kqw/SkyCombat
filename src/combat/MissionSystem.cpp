#include "MissionSystem.h"

namespace sky {

MissionSystem::MissionSystem(size_t totalEnemies) {
    reset(totalEnemies);
}

void MissionSystem::reset(size_t totalEnemies) {
    status_ = MissionStatus::Active;
    totalEnemies_ = totalEnemies;
    enemiesRemaining_ = totalEnemies;
    missionTime_ = 0.0;
}

void MissionSystem::update(double dt, const Aircraft& player, const std::vector<EnemyAI>& enemies) {
    if (dt > 0.0) {
        missionTime_ += dt;
    }

    size_t aliveCount = 0;
    for (const auto& e : enemies) {
        if (e.isAlive()) {
            ++aliveCount;
        }
    }
    enemiesRemaining_ = aliveCount;

    if (status_ == MissionStatus::Active) {
        if (player.health <= 0.0 || player.crashed) {
            status_ = MissionStatus::Failed;
        } else if (enemiesRemaining_ == 0) {
            status_ = MissionStatus::Cleared;
        }
    }
}

} // namespace sky
