#include "DSP_utilities/Oscillator.h"

#include <cmath>

void Oscillator::setFrequency(float freqHz, float sampleRate) {
    // TODO (Lab 01, Q1): Set phaseIncrement_ (radians advanced per sample).
    // Formula: we compute sin(2pi*f*t) to get a sin waveform with period 1/f at timestep t
    // The speed-up occurs by precomputing the argument which is phase
    // we grow this linearly with a phase increment, to get the arg for the next timestep, we need to add 2pi*f*timestep
    // the time between timesteps is 1/sampleRate hence the formula
    this->phaseIncrement_ = 2.0f * M_PI * freqHz / sampleRate;
}

float Oscillator::nextSample() {
    // TODO (Lab 01, Q2): Return the sine of the current phase.
    float currOutput = std::sin(this->phase_);
    this->phase_ += this->phaseIncrement_;
    // should never exceed 2pi by much so this should complete in one go
    // are there better ways to do this?
    while (this->phase_ >= 2.0f * M_PI) {
        this->phase_ -= 2.0f * M_PI;
    }
    return currOutput;
}
