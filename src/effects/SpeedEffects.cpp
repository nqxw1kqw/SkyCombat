#include "SpeedEffects.h"
#include <algorithm>
#include <cmath>

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

double SpeedEffects::evaluateIntensity(double speed, bool boost) const {
    if (!enabled_ || speed <= threshold_) {
        return 0.0;
    }

    double ramp = (speed - threshold_) / (maxSpeed_ - threshold_);
    if (boost) {
        ramp += 0.20;
    }

    return std::clamp(ramp, 0.0, 1.0);
}

void SpeedEffects::draw(double intensity, int screenWidth, int screenHeight) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    if (!enabled_ || intensity <= 0.0) return;

    int numLines = int(12 + intensity * 28);
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;
    int alpha = std::clamp(int(intensity * 180.0), 0, 255);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    uint32_t color = GetColor(220, 235, 255);

    // Simple radial speed lines radiating from near screen center towards edges
    for (int i = 0; i < numLines; ++i) {
        double angle = (double(i) / numLines) * 6.283185307179586;
        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        double innerR = 200.0 + (i % 5) * 25.0;
        double outerR = 500.0 + (i % 7) * 40.0;
        int x1 = cx + int(cosA * innerR);
        int y1 = cy + int(sinA * innerR);
        int x2 = cx + int(cosA * outerR);
        int y2 = cy + int(sinA * outerR);
        DrawLine(x1, y1, x2, y2, color);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
#else
    (void)intensity;
    (void)screenWidth;
    (void)screenHeight;
#endif
}

} // namespace sky
