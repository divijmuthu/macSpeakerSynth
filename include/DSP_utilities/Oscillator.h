#pragma once

#include <cstdint>

// Lab 01: Phase-accumulator oscillator (sine only for now).
//
// Mental model:
//   setFrequency()  -> compute how many radians to advance per sample
//   nextSample()    -> sin(phase), then advance + wrap phase
//
// See learning/lab01/README.md for the fill-in questions.

class Oscillator {
public:
    void setFrequency(float freqHz, float sampleRate);

    // Generate one sample and advance internal phase.
    float nextSample();

    // For tests: read phase without generating a sample.
    float phase() const { return phase_; }

    // For tests: read the per-sample step set by setFrequency.
    float phaseIncrement() const { return phaseIncrement_; }

    void resetPhase() { phase_ = 0.0f; }

private:
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
};
