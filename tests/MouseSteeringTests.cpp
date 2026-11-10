#include "Test.h"
#include "core/MouseSteering.h"
using namespace sky;

// Direct Mouse Flight: pixel deltas in, radians out. Sums a command
// stream over many zero-delta steps so the smoothing filter drains fully.
static double drainYaw(MouseSteering& ms, double dt = 1.0 / 60) {
    double total = 0;
    for (int i = 0; i < 600; ++i) total += ms.update(0, 0, dt).mouseYaw;
    return total;
}
static double drainPitch(MouseSteering& ms, double dt = 1.0 / 60) {
    double total = 0;
    for (int i = 0; i < 600; ++i) total += ms.update(0, 0, dt).mousePitch;
    return total;
}

int main() {
    Tests t;
    const MouseFlightConfig cfg; // defaults: 0.0022 rad/px, clamp 0.35 rad

    {   // A still mouse commands exactly nothing, forever.
        MouseSteering ms;
        bool allZero = true;
        for (int i = 0; i < 120; ++i) {
            auto cmd = ms.update(0, 0, 1.0 / 60);
            allZero = allZero && cmd.mouseYaw == 0.0 && cmd.mousePitch == 0.0;
        }
        t.check(allZero, "still mouse is exactly zero");
    }
    {   // 100 px right yaws right by 100 * sensitivity in total.
        MouseSteering ms;
        double total = ms.update(100, 0, 1.0 / 60).mouseYaw + drainYaw(ms);
        t.check(near(total, 100 * cfg.mouseSensitivityX, 0.005) && total > 0,
                "mouse right yaws right, total conserved");
    }
    {   // Mouse up (negative screen deltaY) pitches the nose up.
        MouseSteering ms;
        double total = ms.update(0, -100, 1.0 / 60).mousePitch + drainPitch(ms);
        t.check(near(total, 100 * cfg.mouseSensitivityY, 0.005) && total > 0,
                "mouse up pitches nose up");
    }
    {   // Mouse down pitches the nose down.
        MouseSteering ms;
        double total = ms.update(0, 100, 1.0 / 60).mousePitch + drainPitch(ms);
        t.check(near(total, -100 * cfg.mouseSensitivityY, 0.005),
                "mouse down pitches nose down");
    }
    {   // invertY flips pitch only.
        MouseFlightConfig c = cfg;
        c.invertY = true;
        MouseSteering ms(c);
        double total = ms.update(0, -100, 1.0 / 60).mousePitch + drainPitch(ms);
        t.check(near(total, -100 * cfg.mouseSensitivityY, 0.005),
                "invertY flips pitch sign");
    }
    {   // Sensitivity scales the total angle linearly.
        MouseFlightConfig c = cfg;
        c.mouseSensitivityX = cfg.mouseSensitivityX * 2;
        MouseSteering ms(c);
        double total = ms.update(50, 0, 1.0 / 60).mouseYaw + drainYaw(ms);
        t.check(near(total, 100 * cfg.mouseSensitivityX, 0.005),
                "sensitivity scales total rotation");
    }
    {   // A huge spike (focus jump) clamps to maxDeltaPerFrame per frame.
        MouseSteering ms;
        double total = ms.update(100000, 0, 1.0 / 60).mouseYaw + drainYaw(ms);
        t.check(near(total, cfg.maxDeltaPerFrame, 0.005),
                "spike clamps to per-frame max");
    }
    {   // Smoothing spreads a flick: the first frame applies only a
        // fraction, but nothing is lost in total.
        MouseSteering ms;
        auto first = ms.update(100, 0, 1.0 / 60);
        double total = first.mouseYaw + drainYaw(ms);
        t.check(first.mouseYaw > 0 && first.mouseYaw < 0.5 * 100 * cfg.mouseSensitivityX &&
                near(total, 100 * cfg.mouseSensitivityX, 0.005),
                "smoothing spreads without loss");
    }
    {   // Total applied rotation does not depend on frame rate.
        MouseSteering a, b;
        double ta = a.update(120, -60, 1.0 / 30).mouseYaw;
        ta += drainYaw(a, 1.0 / 30);
        double tb = b.update(120, -60, 1.0 / 144).mouseYaw;
        tb += drainYaw(b, 1.0 / 144);
        t.check(near(ta, tb, 0.005) && near(ta, 120 * cfg.mouseSensitivityX, 0.005),
                "total rotation is frame-rate independent");
    }
    {   // Reset discards pending rotation immediately.
        MouseSteering ms;
        ms.update(500, 500, 1.0 / 60);
        ms.reset();
        double total = drainYaw(ms) + drainPitch(ms);
        t.check(near(total, 0.0, 1e-9), "reset discards pending rotation");
    }

    return t.finish();
}
