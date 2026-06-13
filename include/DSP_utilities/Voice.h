#pragma once

#include "DSP_utilities/Envelope.h"
#include "DSP_utilities/Oscillator.h"

// Lab 02: One synthesizer voice = oscillator × envelope.
// Lab 03's AudioCore will hold a Voice (later Voice[]) and call nextSample()
// inside the miniaudio callback. This is the exact per-note kernel.

class Voice {
public:
    void setFrequency(float freqHz, float sampleRate) {
        osc_.setFrequency(freqHz, sampleRate);
    }

    void configureEnvelope(float attackSec,
                           float decaySec,
                           float sustainLevel,
                           float releaseSec,
                           float sampleRate) {
        env_.configure(attackSec, decaySec, sustainLevel, releaseSec, sampleRate);
    }

    void noteOn() { env_.noteOn(); }
    void noteOff() { env_.noteOff(); }

    // One output sample — called once per frame in the audio callback.
    float nextSample();

    const Oscillator& oscillator() const { return osc_; }
    const Envelope& envelope() const { return env_; }

private:
    Oscillator osc_;
    Envelope env_;
};
