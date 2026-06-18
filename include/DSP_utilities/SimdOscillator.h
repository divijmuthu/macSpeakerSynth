#pragma once

#include "DSP_utilities/Waveform.h"

#include <cstddef>

class Oscillator;

// Lab 10: process up to `count` samples (4-wide on ARM NEON when available).
// Returns number of samples written (always `count` on success).
std::size_t oscillatorRenderBatch(Oscillator& osc,
                                  Waveform waveform,
                                  float* output,
                                  std::size_t count);
