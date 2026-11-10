#include "FlightController.h"
#include "world/HeightProvider.h"
#include <algorithm>
#include <cmath>

namespace sky {

void FlightController::update(Aircraft& a, const FlightInput& input, double dt, const IHeightProvider* terrain) const {
    if (dt <= 0) return;

    if (terrain && a.crashed) {
        a.speed = 0;
        double groundY = terrain->groundHeight(a.transform.position.x, a.transform.position.z);
        if (a.transform.position.y < groundY) a.transform.position.y = groundY;
        a.clearance = 0;
        return;
    }

    const double pitch = std::clamp(input.pitch, -1.0, 1.0);
    const double turn = std::clamp(input.turn, -1.0, 1.0);
    const double roll = std::clamp(input.roll, -1.0, 1.0);
    // Manual roll (A/D) couples into a gentle coordinated yaw so rolling
    // still steers the aircraft; mouse/keyboard yaw remains primary.
    const double turnEff = std::clamp(turn - roll * 0.5, -1.0, 1.0);
    const double throttle = std::clamp(input.throttle, -1.0, 1.0);

    a.speed = std::clamp(a.speed + throttle * (throttle >= 0 ? FlightTuning::acceleration : FlightTuning::deceleration) * dt,
                         FlightTuning::minSpeed, FlightTuning::maxSpeed);

    // World-up yaw, then local-right pitch. W uses negative X rotation to raise +Z.
    a.transform.orientation = (Quat::axisAngle({0, 1, 0}, turnEff * FlightTuning::yawRate * dt) *
                               a.transform.orientation *
                               Quat::axisAngle({1, 0, 0}, -pitch * FlightTuning::pitchRate * dt)).normalized();

    // Direct mouse rotation: applied once per step, in the same axes and
    // sign conventions as the rate commands above. A still mouse adds
    // exactly nothing, so the aircraft holds its heading.
    if (input.mouseYaw != 0.0 || input.mousePitch != 0.0) {
        a.transform.orientation = (Quat::axisAngle({0, 1, 0}, input.mouseYaw) *
                                   a.transform.orientation *
                                   Quat::axisAngle({1, 0, 0}, -input.mousePitch)).normalized();
    }

    if (terrain) {
        // 1. Boundary soft turn if nearing edge while heading outward
        constexpr double softLimit = 900.0;
        Vec3 pos = a.transform.position;
        Vec3 fwd = a.transform.forward();
        if (std::abs(pos.x) > softLimit || std::abs(pos.z) > softLimit) {
            Vec3 toCenter = normalized(Vec3{-pos.x, 0, -pos.z});
            double outwardHeading = pos.x * fwd.x + pos.z * fwd.z;
            if (outwardHeading > 0) {
                // Steer towards center around world Y
                double steerDir = fwd.z * toCenter.x - fwd.x * toCenter.z;
                double turnSign = (steerDir >= 0.0) ? 1.0 : -1.0;
                Quat boundYaw = Quat::axisAngle({0, 1, 0}, turnSign * FlightTuning::yawRate * dt);
                a.transform.orientation = (boundYaw * a.transform.orientation).normalized();
            }
        }

        // 2. Terrain avoidance assist
        double groundY = terrain->groundHeight(pos.x, pos.z);
        double clearance = pos.y - groundY;
        a.clearance = clearance;
        a.terrainWarning = (clearance < FlightTuning::safeClearance);

        double forwardY = a.transform.forward().y;
        if (clearance <= FlightTuning::warningClearance && forwardY < -1e-4) {
            // Descending towards ground within 25m: apply limited pitch-up assist
            a.terrainAssist = true;
            double descentSeverity = -forwardY;
            double t = std::clamp((FlightTuning::warningClearance - clearance) /
                                  (FlightTuning::warningClearance - FlightTuning::safeClearance), 0.0, 1.0);
            double assistPitch = t * descentSeverity * FlightTuning::maxAssistPitchRate * dt;
            Quat assistRot = Quat::axisAngle({1, 0, 0}, -assistPitch);
            a.transform.orientation = (a.transform.orientation * assistRot).normalized();
        } else {
            a.terrainAssist = false;
        }
    }

    // Bank: a held manual roll wins over the turn-induced auto-bank; with
    // neither input the bank eases back to level on its own. Mouse yaw
    // counts towards the auto-bank as its equivalent turn rate this step.
    const double mouseTurnEq = dt > 0.0 ? std::clamp(input.mouseYaw / (FlightTuning::yawRate * dt), -1.0, 1.0) : 0.0;
    const double bankTurn = std::clamp(turnEff + mouseTurnEq, -1.0, 1.0);
    const double bankTarget = std::abs(roll) > 0.001 ? roll * FlightTuning::maxBank
                                                     : -bankTurn * FlightTuning::maxBank;
    a.visualBank += (bankTarget - a.visualBank) * response(FlightTuning::levelResponse, dt);
    a.transform.position = a.transform.position + a.transform.forward() * (a.speed * dt);

    if (terrain) {
        // Containment to hard world boundary
        a.transform.position.x = std::clamp(a.transform.position.x, WorldConfig::minX, WorldConfig::maxX);
        a.transform.position.z = std::clamp(a.transform.position.z, WorldConfig::minZ, WorldConfig::maxZ);

        double newGroundY = terrain->groundHeight(a.transform.position.x, a.transform.position.z);
        a.clearance = a.transform.position.y - newGroundY;

        if (a.transform.position.y <= newGroundY) {
            // Ground collision: consistent crash state, cannot sink below mesh
            a.crashed = true;
            a.speed = 0.0;
            a.transform.position.y = newGroundY;
            a.clearance = 0.0;
            a.terrainWarning = true;
            a.terrainAssist = false;
        } else {
            a.terrainWarning = (a.clearance < FlightTuning::safeClearance);
        }
    }
}

} // namespace sky
