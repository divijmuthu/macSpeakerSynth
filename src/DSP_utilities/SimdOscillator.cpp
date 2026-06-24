#include "DSP_utilities/SimdOscillator.h"

#include "DSP_utilities/Oscillator.h"

#include <cmath>

#if defined(RACE_NEON)
#include <arm_neon.h>
#endif

namespace {

constexpr float kTwoPi = 6.283185307179586f;
constexpr float kPi = 3.14159265358979323846f;

void wrapPhaseScalar(float& phase) {
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
    while (phase < 0.0f) {
        phase += kTwoPi;
    }
}

#if defined(RACE_NEON)

// Wrap each lane into [0, 2pi). We use a tiny scalar lane loop for correctness.
float32x4_t wrapPhaseVec(float32x4_t phaseVec) {
    alignas(16) float phases[4];
    vst1q_f32(phases, phaseVec);
    for (float& p : phases) {
        wrapPhaseScalar(p);
    }
    return vld1q_f32(phases);
}

float32x4_t neonWaveformSample(float32x4_t wrappedPhase, Waveform waveform) {
    switch (waveform) {
        case Waveform::Saw: {
            const float32x4_t scale = vdupq_n_f32(2.0f / kTwoPi);
            const float32x4_t one = vdupq_n_f32(1.0f);
            return vsubq_f32(vmulq_f32(wrappedPhase, scale), one);
        }
        case Waveform::Square: {
            const uint32x4_t mask = vcltq_f32(wrappedPhase, vdupq_n_f32(kPi));
            const float32x4_t plusOne = vdupq_n_f32(1.0f);
            const float32x4_t minusOne = vdupq_n_f32(-1.0f);
            return vbslq_f32(mask, plusOne, minusOne);
        }
        case Waveform::Triangle: {
            const float32x4_t phaseOverPi = vmulq_f32(wrappedPhase, vdupq_n_f32(1.0f / kPi));
            const float32x4_t left = vsubq_f32(vmulq_f32(vdupq_n_f32(2.0f), phaseOverPi),
                                               vdupq_n_f32(1.0f));
            const float32x4_t right = vsubq_f32(vdupq_n_f32(3.0f),
                                                vmulq_f32(vdupq_n_f32(2.0f), phaseOverPi));
            const uint32x4_t mask = vcltq_f32(wrappedPhase, vdupq_n_f32(kPi));
            return vbslq_f32(mask, left, right);
        }
        case Waveform::Sine:
        default: {
            // No portable NEON sin intrinsic; evaluate per lane.
            alignas(16) float phases[4];
            alignas(16) float out[4];
            vst1q_f32(phases, wrappedPhase);
            for (int i = 0; i < 4; ++i) {
                out[i] = std::sin(phases[i]);
            }
            return vld1q_f32(out);
        }
    }
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
        float32x4_t phaseVec = vaddq_f32(vdupq_n_f32(phase), offsets);
        phaseVec = wrapPhaseVec(phaseVec);
        const float32x4_t samples = neonWaveformSample(phaseVec, waveform);
        vst1q_f32(output + i, samples);

        phase += 4.0f * inc;
        wrapPhaseScalar(phase);
    }
#endif

    for (; i < count; ++i) {
        const float wrappedPhase = phase;
        output[i] = waveformSample(wrappedPhase, waveform);
        phase += inc;
        wrapPhaseScalar(phase);
    }

    // Keep oscillator state consistent with the consumed batch.
    osc.syncPhase(phase);
    return count;
}
