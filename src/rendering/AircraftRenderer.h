#pragma once
#include "aircraft/Aircraft.h"

namespace sky {

struct Bullet;
class Missile;

class AircraftRenderer {
public:
    void draw(const Aircraft& player) const;
    void drawEnemy(const Aircraft& enemy, bool isTargeted = false, bool isLocked = false) const;
    void drawBullet(const Bullet& bullet) const;
    void drawMissile(const Missile& missile) const;
};

} // namespace sky
