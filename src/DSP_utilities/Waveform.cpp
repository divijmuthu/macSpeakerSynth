#include "DSP_utilities/Waveform.h"

#include <cmath>

namespace {

constexpr float kTwoPi = 6.283185307179586f;
constexpr float kPi = 3.14159265358979323846f;

}  // namespace

float waveformSample(float phase, Waveform waveform) {
    switch (waveform) {
        case Waveform::Sine:
            return std::sin(phase);
        case Waveform::Saw:
            // Rising ramp −1 → +1 across one cycle.
            return 2.0f * (phase / kTwoPi) - 1.0f;
        case Waveform::Square:
            return phase < kPi ? 1.0f : -1.0f;
        case Waveform::Triangle:
            // 0→π rise −1→+1, π→2π fall +1→−1
            if (phase < kPi) {
                return 2.0f * (phase / kPi) - 1.0f;
            }
            return 3.0f - 2.0f * (phase / kPi);
    }
    return std::sin(phase);
}
