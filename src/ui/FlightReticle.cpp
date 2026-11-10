#include "FlightReticle.h"

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

void FlightReticle::draw(int centerX, int centerY, int halfWidth, int halfHeight,
                         double rawNX, double rawNY, double deadzone, bool showDeadzone) {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    const unsigned int crossColor = GetColor(120, 255, 170);
    // Fixed centre cross (the aircraft's current heading reference).
    DrawLine(centerX - 14, centerY, centerX - 5, centerY, crossColor);
    DrawLine(centerX + 5, centerY, centerX + 14, centerY, crossColor);
    DrawLine(centerX, centerY - 14, centerX, centerY - 5, crossColor);
    DrawLine(centerX, centerY + 5, centerX, centerY + 14, crossColor);
    DrawCircle(centerX, centerY, 3, crossColor, FALSE);

    // Steering marker at the raw cursor position.
    const int mx = centerX + int(rawNX * halfWidth);
    const int my = centerY - int(rawNY * halfHeight);
    DrawCircle(mx, my, 7, GetColor(255, 220, 90), FALSE);
    DrawCircle(mx, my, 2, GetColor(255, 220, 90), TRUE);

    if (showDeadzone) {
        DrawOval(centerX, centerY, int(deadzone * halfWidth), int(deadzone * halfHeight),
                 GetColor(140, 160, 180), FALSE);
    }
#else
    (void)centerX; (void)centerY; (void)halfWidth; (void)halfHeight;
    (void)rawNX; (void)rawNY; (void)deadzone; (void)showDeadzone;
#endif
}

} // namespace sky
