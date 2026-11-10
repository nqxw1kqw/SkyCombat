#include "AudioModel.h"
#include <algorithm>
#include <cmath>

#if !defined(SKY_PORTABLE) && defined(_WIN32)
#include <DxLib.h>
#endif

namespace sky {

AudioParameters AudioModel::evaluate(double speed, double throttle, bool boost) const {
    AudioParameters p;

    // Speed factor normalized to [0, 1] between 25 m/s and 140 m/s
    double s = std::clamp((speed - 25.0) / (140.0 - 25.0), 0.0, 1.0);
    double th = std::clamp(throttle, -1.0, 1.0);

    // 1. Engine volume: 0.35 at idle/min speed to 0.95 at max speed, +0.05 on boost
    double baseVol = 0.35 + 0.60 * s;
    if (th > 0.0) baseVol += 0.05 * th;
    if (boost) baseVol += 0.05;
    p.engineVolume = std::clamp(baseVol, 0.0, 1.0);

    // 2. Engine pitch: 0.80 at idle to 1.60 at max speed, +0.10 on boost
    double basePitch = 0.80 + 0.80 * s;
    if (th > 0.0) basePitch += 0.05 * th;
    if (boost) basePitch += 0.10;
    p.enginePitch = std::clamp(basePitch, 0.5, 2.0);

    // 3. Wind volume: gentle below cruise (70 m/s), roaring at 140 m/s
    double windFactor = std::clamp((speed - 35.0) / (140.0 - 35.0), 0.0, 1.0);
    double windVol = windFactor * windFactor * 0.90;
    if (boost) windVol += 0.10;
    p.windVolume = std::clamp(windVol, 0.0, 1.0);

    return p;
}

void AudioModel::updateDXLib(int engineSoundHandle, int windSoundHandle, const AudioParameters& params) const {
#if !defined(SKY_PORTABLE) && defined(_WIN32)
    if (engineSoundHandle != -1 && CheckSoundMem(engineSoundHandle) == 1) {
        ChangeVolumeSoundMem(int(std::clamp(params.engineVolume, 0.0, 1.0) * 255.0), engineSoundHandle);
        SetFrequencySoundMem(int(44100.0 * std::clamp(params.enginePitch, 0.5, 2.0)), engineSoundHandle);
    }
    if (windSoundHandle != -1 && CheckSoundMem(windSoundHandle) == 1) {
        ChangeVolumeSoundMem(int(std::clamp(params.windVolume, 0.0, 1.0) * 255.0), windSoundHandle);
    }
#else
    (void)engineSoundHandle;
    (void)windSoundHandle;
    (void)params;
#endif
}

} // namespace sky
