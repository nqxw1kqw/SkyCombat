#include "Aircraft.h"

namespace sky {

void Aircraft::reset() {
    transform = {{0, 120, 0}, {}};
    speed = 70;
    health = 100;
    visualBank = 0;
    clearance = 120;
    terrainWarning = false;
    terrainAssist = false;
    crashed = false;
}

Aircraft interpolate(const Aircraft& a, const Aircraft& b, double t) {
    Aircraft result = b;
    result.transform = interpolate(a.transform, b.transform, t);
    result.speed = a.speed + (b.speed - a.speed) * t;
    result.visualBank = a.visualBank + (b.visualBank - a.visualBank) * t;
    result.clearance = a.clearance + (b.clearance - a.clearance) * t;
    result.terrainWarning = b.terrainWarning;
    result.terrainAssist = b.terrainAssist;
    result.crashed = b.crashed;
    return result;
}

}
