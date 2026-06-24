#include "DSP_utilities/Voice.h"
#include "DSP_utilities/SimdOscillator.h"

namespace {
constexpr float kTwoPi = 6.283185307179586f;
}  // namespace

Voice::Voice() {
    setCutoff(12000.0f, 48000.0f);
}

void Voice::setFrequency(float freqHz, float sampleRate) {
    frequencyHz_ = freqHz;
    osc_.setFrequency(freqHz, sampleRate);
    simdReadIndex_ = 0;
    simdValidCount_ = 0;
}

void Voice::setWaveform(Waveform waveform) {
    osc_.setWaveform(waveform);
    simdReadIndex_ = 0;
    simdValidCount_ = 0;
}

void Voice::refillSimdBatch() {
    simdValidCount_ = oscillatorRenderBatch(osc_, osc_.waveform(), simdBatch_.data(), kSimdBatchSize);
    simdReadIndex_ = 0;
}

float Voice::nextRandomPhase() {
    // Xorshift32: tiny deterministic RNG for phase decorrelation.
    phaseRng_ ^= phaseRng_ << 13;
    phaseRng_ ^= phaseRng_ >> 17;
    phaseRng_ ^= phaseRng_ << 5;
    const float unit = static_cast<float>(phaseRng_) / 4294967295.0f;
    return unit * kTwoPi;
}

void Voice::noteOn() {
    // Fresh note from silence: reset filter and randomize phase so chord voices
    // are less phase-locked (reduces buzzy/comb-like stacking).
    if (!env_.isActive()) {
        osc_.syncPhase(nextRandomPhase());
        filter_.reset();
    }
    simdReadIndex_ = 0;
    simdValidCount_ = 0;
    env_.noteOn();
}

float Voice::nextSample() {
    const bool wasActive = env_.isActive();
    const float gain = env_.process();
    float osc = 0.0f;
    if (simdEnabled_) {
        if (simdReadIndex_ >= simdValidCount_) {
            refillSimdBatch();
        }
        if (simdReadIndex_ < simdValidCount_) {
            osc = simdBatch_[simdReadIndex_++];
        }
    } else {
        osc = osc_.nextSample();
    }

    if (wasActive && !env_.isActive()) {
        filter_.reset();
    }
    if (gain <= 0.0f) {
        return 0.0f;
    }

    // Filter the raw oscillator, then apply envelope (VCA). Multiplying last
    // guarantees output hits true zero when gain is zero — filter memory alone
    // cannot leak audio or pop on retrigger.
    return filter_.process(osc) * gain;
}
