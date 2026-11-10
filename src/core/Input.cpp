#include "Input.h"
namespace sky {
InputState Input::update(const KeyboardState& k) {
    InputState result{{double(k.w)-double(k.s),double(k.d)-double(k.a),
                       double(k.shift)-double(k.ctrl),k.shift&&!k.ctrl},k.reset&&!resetHeld_,k.escape};
    resetHeld_=k.reset;
    return result;
}
}
