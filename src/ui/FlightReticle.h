#pragma once

namespace sky {

// Fixed centre aim reticle. Direct Mouse Flight is relative — the cursor
// is captured and re-centred every frame, so there is no cursor offset to
// mark; the cross is the aircraft's heading reference.
class FlightReticle {
public:
    static void draw(int centerX, int centerY);
};

} // namespace sky
