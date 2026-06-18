#pragma once

#include "DSP_utilities/Envelope.h"
#include "DSP_utilities/Oscillator.h"
#include "DSP_utilities/StateVariableFilter.h"
#include "DSP_utilities/Waveform.h"

// Lab 02: oscillator × envelope
// Lab 06: → TPT state-variable filter
// Lab 08: isActive + frequency tracking for polyphony

class Voice {
public:
    Voice();

    void setFrequency(float freqHz, float sampleRate) {
        frequencyHz_ = freqHz;
        osc_.setFrequency(freqHz, sampleRate);
    }

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

    void setWaveform(Waveform waveform) { osc_.setWaveform(waveform); }
    Waveform waveform() const { return osc_.waveform(); }

    void noteOn();
    void noteOff() { env_.noteOff(); }

    bool isActive() const { return env_.isActive(); }

    float nextSample();

    const Oscillator& oscillator() const { return osc_; }
    const Envelope& envelope() const { return env_; }
    const StateVariableFilter& filter() const { return filter_; }

private:
    Oscillator osc_;
    Envelope env_;
    StateVariableFilter filter_;
    float frequencyHz_ = 440.0f;
};
