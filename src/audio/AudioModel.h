#pragma once
#include <algorithm>

namespace sky {

struct AudioParameters {
    double engineVolume{0.0}; // [0.0, 1.0]
    double enginePitch{1.0};  // [0.5, 2.0]
    double windVolume{0.0};   // [0.0, 1.0]
};

class AudioModel {
public:
    // Pure mapping from flight parameters to engine/wind audio parameters
    AudioParameters evaluate(double speed, double throttle = 0.0, bool boost = false) const;

    // DXLib sound update layer: only applies to valid loaded sound handles
    void updateDXLib(int engineSoundHandle, int windSoundHandle, const AudioParameters& params) const;
};

} // namespace sky
