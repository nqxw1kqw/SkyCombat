#pragma once
#include "core/Transform.h"
namespace sky {
struct Aircraft {
    Transform transform;
    double speed{70};
    double health{100}; // Reserved state; no combat in M1.
    double visualBank{};
    void reset();
};
Aircraft interpolate(const Aircraft& previous,const Aircraft& current,double alpha);
}
