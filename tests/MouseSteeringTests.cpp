#include "Test.h"
#include "core/MouseSteering.h"
using namespace sky;

int main() {
    Tests t;

    {   // Cursor at the centre commands nothing.
        MouseSteering ms;
        auto cmd = ms.update(0.0, 0.0, 1.0 / 60);
        t.check(near(cmd.pitch, 0) && near(cmd.yaw, 0), "centre is neutral");
    }
    {   // Inside the radial deadzone the command stays zero.
        MouseSteering ms;
        auto cmd = ms.update(0.05, -0.04, 1.0 / 60);
        t.check(near(cmd.pitch, 0) && near(cmd.yaw, 0), "deadzone swallows small offsets");
    }
    {   // Full right deflection settles to yaw +1 (turn right), no pitch.
        MouseSteering ms;
        MouseSteering::Command cmd{};
        for (int i = 0; i < 240; ++i) cmd = ms.update(1.0, 0.0, 1.0 / 60);
        t.check(near(cmd.yaw, 1.0, 1e-6) && near(cmd.pitch, 0), "full right is yaw +1");
    }
    {   // Full up deflection settles to pitch +1 (nose up, project convention).
        MouseSteering ms;
        MouseSteering::Command cmd{};
        for (int i = 0; i < 240; ++i) cmd = ms.update(0.0, 1.0, 1.0 / 60);
        t.check(near(cmd.pitch, 1.0, 1e-6) && near(cmd.yaw, 0), "full up is pitch +1");
    }
    {   // Response curve: half deflection maps below half command.
        MouseSteering ms;
        MouseSteering::Command cmd{};
        for (int i = 0; i < 240; ++i) cmd = ms.update(0.5, 0.0, 1.0 / 60);
        const double expected = 0.3335; // ((0.5-0.08)/0.92)^1.4
        t.check(near(cmd.yaw, expected, 0.01) && cmd.yaw < 0.5, "response curve softens mid range");
    }
    {   // Smoothing is frame-rate independent: 1 x 100ms ~ 10 x 10ms.
        MouseSteering a, b;
        auto ca = a.update(1.0, 0.0, 0.1);
        MouseSteering::Command cb{};
        for (int i = 0; i < 10; ++i) cb = b.update(1.0, 0.0, 0.01);
        t.check(near(ca.yaw, cb.yaw, 0.02), "smoothing matches across frame rates");
    }
    {   // Out-of-range cursor positions clamp into [-1, 1] commands.
        MouseSteering ms;
        MouseSteering::Command cmd{};
        for (int i = 0; i < 240; ++i) cmd = ms.update(2.0, -2.0, 1.0 / 60);
        t.check(cmd.yaw <= 1.0 && cmd.yaw > 0.5 && cmd.pitch >= -1.0 && cmd.pitch < -0.5,
                "commands clamp to unit range");
    }
    {   // invertMouseY flips pitch only.
        MouseFlightConfig cfg;
        cfg.invertMouseY = true;
        MouseSteering ms(cfg);
        MouseSteering::Command cmd{};
        for (int i = 0; i < 240; ++i) cmd = ms.update(0.0, 1.0, 1.0 / 60);
        t.check(near(cmd.pitch, -1.0, 1e-6), "invert Y flips pitch sign");
    }
    {   // Reset returns the command to neutral immediately.
        MouseSteering ms;
        for (int i = 0; i < 60; ++i) ms.update(1.0, 1.0, 1.0 / 60);
        ms.reset();
        auto cmd = ms.update(0.0, 0.0, 1.0 / 60);
        t.check(near(cmd.pitch, 0) && near(cmd.yaw, 0), "reset clears smoothing");
    }

    return t.finish();
}
