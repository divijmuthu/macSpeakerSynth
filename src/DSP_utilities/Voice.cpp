#include "DSP_utilities/Voice.h"

// This is considered a "synth voice" so the core combo of osc + envelope to play a sound

float Voice::nextSample() {
    // TODO (Lab 02, Q4): Combine oscillator and envelope — the core synth multiply.
    //
    //   gain = env_.process();
    //   raw  = osc_.nextSample();
    //   return raw * gain;
    //
    // Order matters: process envelope first so gain matches this time step.
    // grab envelope's gain based on current state + time step
    const float gain = this->env_.process();
    // compute the waveform sample from the oscillator
    const float raw = this->osc_.nextSample();
    // scale waveform sample by envelope gain
    return raw * gain;
}
