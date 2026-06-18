#pragma once

#include "DSP_utilities/Waveform.h"

#include <cstdint>

// Lab 01 / 09: Phase-accumulator oscillator (sine + saw/square/triangle).

class Oscillator {
public:
    void setFrequency(float freqHz, float sampleRate);
    void setWaveform(Waveform waveform) { waveform_ = waveform; }
    Waveform waveform() const { return waveform_; }

    float nextSample();

    float phase() const { return phase_; }
    float phaseIncrement() const { return phaseIncrement_; }

    void resetPhase() { phase_ = 0.0f; }

    // Lab 10: set wrapped phase after SIMD batch.
    void syncPhase(float phaseRadians);

private:
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    Waveform waveform_ = Waveform::Sine;
};
