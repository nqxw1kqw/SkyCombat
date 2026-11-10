#pragma once

namespace sky {

// 2D steering reticle for mouse flight: a fixed centre cross, a marker at
// the raw cursor offset, and (optionally) the deadzone oval. Coordinates
// are normalised (x right, y up, [-1, 1] from the window centre).
class FlightReticle {
public:
    static void draw(int centerX, int centerY, int halfWidth, int halfHeight,
                     double rawNX, double rawNY, double deadzone, bool showDeadzone);
};

} // namespace sky
