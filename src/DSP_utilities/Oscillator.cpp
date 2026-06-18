#include "DSP_utilities/Oscillator.h"

#include <cmath>

void Oscillator::setFrequency(float freqHz, float sampleRate) {
    phaseIncrement_ = 2.0f * static_cast<float>(M_PI) * freqHz / sampleRate;
}

float Oscillator::nextSample() {
    const float sample = waveformSample(phase_, waveform_);
    phase_ += phaseIncrement_;
    while (phase_ >= 2.0f * static_cast<float>(M_PI)) {
        phase_ -= 2.0f * static_cast<float>(M_PI);
    }
    return sample;
}

void Oscillator::syncPhase(float phaseRadians) {
    phase_ = phaseRadians;
    while (phase_ >= 2.0f * static_cast<float>(M_PI)) {
        phase_ -= 2.0f * static_cast<float>(M_PI);
    }
}
