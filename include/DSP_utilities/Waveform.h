#pragma once

#include <cstdint>

// Lab 09: oscillator waveshapes (phase-accumulator family).

enum class Waveform : std::uint8_t {
    Sine = 0,
    Saw = 1,
    Square = 2,
    Triangle = 3,
};

constexpr const char* waveformName(Waveform wf) {
    switch (wf) {
        case Waveform::Sine:
            return "Sine";
        case Waveform::Saw:
            return "Saw";
        case Waveform::Square:
            return "Square";
        case Waveform::Triangle:
            return "Triangle";
    }
    return "Sine";
}

// One sample from phase in [0, 2π). Shared by scalar and SIMD paths.
float waveformSample(float phase, Waveform waveform);
