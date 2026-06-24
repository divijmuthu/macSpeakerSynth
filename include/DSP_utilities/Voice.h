#pragma once

#include "DSP_utilities/Envelope.h"
#include "DSP_utilities/Oscillator.h"
#include "DSP_utilities/StateVariableFilter.h"
#include "DSP_utilities/Waveform.h"

#include <array>
#include <cstddef>

// Lab 02: oscillator × envelope
// Lab 06: → TPT state-variable filter
// Lab 08: isActive + frequency tracking for polyphony

class Voice {
public:
    Voice();

    void setFrequency(float freqHz, float sampleRate);

    float frequencyHz() const { return frequencyHz_; }

    void configureEnvelope(float attackSec,
                           float decaySec,
                           float sustainLevel,
                           float releaseSec,
                           float sampleRate) {
        env_.configure(attackSec, decaySec, sustainLevel, releaseSec, sampleRate);
    }

    void setCutoff(float cutoffHz, float sampleRate) {
        filter_.setCutoff(cutoffHz, sampleRate);
    }

    void setFilterMode(FilterMode mode) { filter_.setMode(mode); }

    void setWaveform(Waveform waveform);
    Waveform waveform() const { return osc_.waveform(); }

    void setSimdEnabled(bool enabled) {
        simdEnabled_ = enabled;
        simdReadIndex_ = 0;
        simdValidCount_ = 0;
    }

    void noteOn();
    void noteOff() { env_.noteOff(); }

    bool isActive() const { return env_.isActive(); }

    float nextSample();

    const Oscillator& oscillator() const { return osc_; }
    const Envelope& envelope() const { return env_; }
    const StateVariableFilter& filter() const { return filter_; }

private:
    static constexpr std::size_t kSimdBatchSize = 32;
    void refillSimdBatch();

    float nextRandomPhase();

    Oscillator osc_;
    Envelope env_;
    StateVariableFilter filter_;
    float frequencyHz_ = 440.0f;
    std::uint32_t phaseRng_ = 0x13572468u;
    bool simdEnabled_ = false;
    std::array<float, kSimdBatchSize> simdBatch_{};
    std::size_t simdReadIndex_ = 0;
    std::size_t simdValidCount_ = 0;
};
