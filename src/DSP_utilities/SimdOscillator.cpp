#include "DSP_utilities/SimdOscillator.h"

#include "DSP_utilities/Oscillator.h"

#include <cmath>

#if defined(RACE_NEON)
#include <arm_neon.h>
#endif

namespace {

constexpr float kTwoPi = 6.283185307179586f;
constexpr float kPi = 3.14159265358979323846f;

void wrapPhase(float& phase) {
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
}

#if defined(RACE_NEON)

float32x4_t neonWaveformSample(float32x4_t phase, Waveform waveform) {
    alignas(16) float phases[4];
    alignas(16) float out[4];
    vst1q_f32(phases, phase);

    switch (waveform) {
        case Waveform::Sine:
            for (int i = 0; i < 4; ++i) {
                out[i] = std::sin(phases[i]);
            }
            break;
        case Waveform::Saw:
            for (int i = 0; i < 4; ++i) {
                out[i] = 2.0f * (phases[i] / kTwoPi) - 1.0f;
            }
            break;
        case Waveform::Square:
            for (int i = 0; i < 4; ++i) {
                out[i] = phases[i] < kPi ? 1.0f : -1.0f;
            }
            break;
        case Waveform::Triangle:
            for (int i = 0; i < 4; ++i) {
                out[i] = waveformSample(phases[i], Waveform::Triangle);
            }
            break;
    }
    return vld1q_f32(out);
}

#endif

}  // namespace

std::size_t oscillatorRenderBatch(Oscillator& osc,
                                  Waveform waveform,
                                  float* output,
                                  std::size_t count) {
    float phase = osc.phase();
    const float inc = osc.phaseIncrement();
    std::size_t i = 0;

#if defined(RACE_NEON)
    const float32x4_t offsets = {0.0f, inc, 2.0f * inc, 3.0f * inc};

    for (; i + 4 <= count; i += 4) {
        const float32x4_t p = vaddq_f32(vdupq_n_f32(phase), offsets);
        const float32x4_t samples = neonWaveformSample(p, waveform);
        vst1q_f32(output + i, samples);
        phase += 4.0f * inc;
        wrapPhase(phase);
    }
#endif

    for (std::size_t j = i; j < count; ++j) {
        output[j] = waveformSample(phase, waveform);
        phase += inc;
        wrapPhase(phase);
    }

    osc.syncPhase(phase);
    return count;
}
