#include "FlightReticle.h"

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

void FlightReticle::draw(int centerX, int centerY) {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    const unsigned int crossColor = GetColor(120, 255, 170);
    DrawLine(centerX - 14, centerY, centerX - 5, centerY, crossColor);
    DrawLine(centerX + 5, centerY, centerX + 14, centerY, crossColor);
    DrawLine(centerX, centerY - 14, centerX, centerY - 5, crossColor);
    DrawLine(centerX, centerY + 5, centerX, centerY + 14, crossColor);
    DrawCircle(centerX, centerY, 3, crossColor, FALSE);
#else
    (void)centerX; (void)centerY;
#endif
}

} // namespace sky
