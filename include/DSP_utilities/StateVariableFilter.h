#pragma once

#include <cstdint>

// Lab 06: Trapezoidal (TPT) state-variable filter — LP / HP / BP from one state.
// See learning/lab06/README.md.

enum class FilterMode : std::uint8_t {
    LowPass = 0,
    HighPass = 1,
    BandPass = 2,
};

class StateVariableFilter {
public:
    void setCutoff(float cutoffHz, float sampleRate, float q = 0.707f);
    void setMode(FilterMode mode) { mode_ = mode; }
    FilterMode mode() const { return mode_; }

    float process(float input);
    void reset();

    // For tests: read last computed outputs from the most recent process() call.
    float lastLow() const { return lp_; }
    float lastBand() const { return bp_; }
    float lastHigh() const { return hp_; }

private:
    FilterMode mode_ = FilterMode::LowPass;

    float g_ = 0.0f;   // trigonometric cutoff mapping (tan)
    float k_ = 1.414f; // 1/Q

    float ic1eq_ = 0.0f;
    float ic2eq_ = 0.0f;

    float lp_ = 0.0f;
    float bp_ = 0.0f;
    float hp_ = 0.0f;
};
