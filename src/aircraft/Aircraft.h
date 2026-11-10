#pragma once
#include "core/Transform.h"

namespace sky {

struct Aircraft {
    Transform transform;
    double speed{70};
    double health{100}; // Reserved state; no combat in M1/M2.
    double visualBank{};
    double clearance{120};
    bool terrainWarning{false};
    bool terrainAssist{false};
    bool crashed{false};

    void reset();
};

Aircraft interpolate(const Aircraft& previous, const Aircraft& current, double alpha);

}
