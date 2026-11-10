#include "Input.h"

namespace sky {

InputState Input::update(const KeyboardState& k) {
    InputState result{
        {double(k.w) - double(k.s), double(k.d) - double(k.a),
         double(k.shift) - double(k.ctrl), k.shift && !k.ctrl},
        k.reset && !resetHeld_,
        k.escape,
        k.space,
        k.f && !fHeld_,
        k.tab && !tabHeld_
    };
    resetHeld_ = k.reset;
    fHeld_ = k.f;
    tabHeld_ = k.tab;
    return result;
}

} // namespace sky
