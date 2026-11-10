#include "Radar.h"
#include <algorithm>
#include <cmath>

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

RadarData Radar::createData(const Aircraft& player, const std::vector<EnemyAI>& enemies,
                            int targetedIndex, bool hasLock, double range) {
    RadarData d;
    d.range = (range > 10.0) ? range : 1500.0;
    d.blips.reserve(enemies.size());

    Vec3 playerPos = player.transform.position;
    Vec3 fwd = player.transform.forward();
    Vec3 right = player.transform.right();

    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto& e = enemies[i];
        if (!e.isAlive()) continue;

        Vec3 toEnemy = e.aircraft().transform.position - playerPos;
        double dist = length(toEnemy);

        // Project relative to player heading
        double xRel = dot(toEnemy, right) / d.range;
        double yRel = dot(toEnemy, fwd) / d.range;

        // Clamp to radar circle boundary
        double mag = std::sqrt(xRel * xRel + yRel * yRel);
        if (mag > 1.0) {
            xRel /= mag;
            yRel /= mag;
        }

        RadarBlip blip;
        blip.relX = xRel;
        blip.relY = yRel;
        blip.distance = dist;
        blip.isLocked = (int(i) == targetedIndex && hasLock);
        blip.isTargeted = (int(i) == targetedIndex);
        blip.isAlive = true;
        blip.id = int(i);

        d.blips.push_back(blip);
    }

    return d;
}

void Radar::draw(const RadarData& data) {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    int cx = data.screenX;
    int cy = data.screenY;
    int r = data.radius;

    // Outer circle and inner range ring (50% scale)
    DrawCircle(cx, cy, r, GetColor(40, 60, 50), FALSE);
    DrawCircle(cx, cy, r / 2, GetColor(30, 45, 35), FALSE);

    // Crosshairs
    DrawLine(cx - r, cy, cx + r, cy, GetColor(30, 50, 40));
    DrawLine(cx, cy - r, cx, cy + r, GetColor(30, 50, 40));

    // Player heading triangle at center
    DrawTriangle(cx, cy - 5, cx - 4, cy + 4, cx + 4, cy + 4, GetColor(100, 220, 255), TRUE);

    // Blips
    for (const auto& b : data.blips) {
        int bx = cx + int(b.relX * r);
        int by = cy - int(b.relY * r);

        if (b.isLocked) {
            // Locked target: diamond + yellow box
            DrawBox(bx - 3, by - 3, bx + 3, by + 3, GetColor(255, 220, 50), TRUE);
            DrawCircle(bx, by, 5, GetColor(255, 240, 80), FALSE);
        } else if (b.isTargeted) {
            // Selected target: cyan diamond
            DrawTriangle(bx, by - 4, bx - 3, by, bx + 3, by, GetColor(80, 220, 240), TRUE);
            DrawTriangle(bx, by + 4, bx - 3, by, bx + 3, by, GetColor(80, 220, 240), TRUE);
        } else {
            // Hostile blip: red circle
            DrawCircle(bx, by, 3, GetColor(255, 70, 70), TRUE);
        }
    }
#else
    (void)data;
#endif
}

} // namespace sky
