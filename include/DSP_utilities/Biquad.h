#pragma once

// Lab 05: Second-order IIR biquad (low-pass for subtractive synthesis).
// See learning/lab05/README.md.

class Biquad {
public:
    // Recompute coefficients when cutoff or sample rate changes (call from audio thread).
    void setLowPass(float cutoffHz, float sampleRate, float q = 0.707f);

    // One sample through the filter (Direct Form I).
    float process(float input);

    void reset();

private:
    float b0_ = 1.0f;
    float b1_ = 0.0f;
    float b2_ = 0.0f;
    float a1_ = 0.0f;
    float a2_ = 0.0f;

    float x1_ = 0.0f;
    float x2_ = 0.0f;
    float y1_ = 0.0f;
    float y2_ = 0.0f;
};
