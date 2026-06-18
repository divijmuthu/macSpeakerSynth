#include "DSP_utilities/StateVariableFilter.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

void StateVariableFilter::setCutoff(float cutoffHz, float sampleRate, float q) {
    const float fc = std::clamp(cutoffHz, 20.0f, sampleRate * 0.49f);
    g_ = std::tan(kPi * fc / sampleRate);
    k_ = 1.0f / q;
}

float StateVariableFilter::process(float input) {
    // process then return the appropriate output based on the mode
    // compute v1 and v2 using the equations
    // update ic1eq_ and ic2eq_ using the equations
    // return the appropriate output based on the mode
    const float v1 = (ic1eq_ + g_ * (input - ic2eq_)) / (1.0f + g_ * (g_ + k_));
    const float v2 = ic2eq_ + g_ * v1;
    lp_ = v2; // LPF output is v2
    bp_ = v1; // BPF output is v1
    hp_ = input - k_ * v1 - v2; // HPF output is input - k_ * v1 - v2
    ic1eq_ = 2.0f * v1 - ic1eq_; // update ic1eq_ 
    ic2eq_ = 2.0f * v2 - ic2eq_; // update ic2eq_ 
    // giant ternary operator to return the appropriate output based on the mode
    return mode_ == FilterMode::LowPass ? lp_ : mode_ == FilterMode::HighPass ? hp_ : bp_; 
}

void StateVariableFilter::reset() {
    // brings filter back to initial state, zeroes out all the variables
    ic1eq_ = ic2eq_ = 0.0f;
    lp_ = bp_ = hp_ = 0.0f; 
} 
