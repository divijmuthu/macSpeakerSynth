#pragma once

#include <cstdint>

// Lab 04: Small control messages from Python/ZMQ thread → audio thread.
// Kept trivially copyable so the lock-free queue can store them by value.

enum class ControlType : std::uint8_t {
    NoteOn,
    NoteOff,
    Cutoff,       // frequencyHz = cutoff Hz
    FilterMode,   // frequencyHz = 0 LP, 1 HP, 2 BP
    DelayTime,    // frequencyHz = delay seconds (Lab 07)
    DelayFeedback,
    DelayWet,
    Drive,
    Waveform,  // frequencyHz = 0 sine, 1 saw, 2 square, 3 triangle (Lab 09)
    MasterGain, // frequencyHz = 0..1.5 linear output gain
    SimdMode,   // frequencyHz = 0 scalar, 1 SIMD batch in Voice nextSample
};

struct ControlMessage {
    ControlType type = ControlType::NoteOff;
    float frequencyHz = 440.0f;
};
