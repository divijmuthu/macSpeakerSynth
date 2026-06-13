#pragma once

#include <cstdint>

// Lab 02: ADSR envelope generator (gain 0.0 → 1.0 → sustain → 0.0).
//
// Used inside Voice and called once per audio sample from the callback.
// See learning/lab02/README.md.

enum class EnvelopeStage : std::uint8_t {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release,
};

class Envelope {
public:
    // Times in seconds; sustainLevel in [0, 1].
    void configure(float attackSec,
                   float decaySec,
                   float sustainLevel,
                   float releaseSec,
                   float sampleRate);

    void noteOn();
    void noteOff();

    // Advance one sample; returns current gain in [0, 1].
    float process();

    EnvelopeStage stage() const { return stage_; }
    float level() const { return level_; }
    bool isActive() const { return stage_ != EnvelopeStage::Idle; }

private:
    float attackSec_ = 0.01f;
    float decaySec_ = 0.1f;
    float sustainLevel_ = 0.7f;
    float releaseSec_ = 0.2f;
    float sampleRate_ = 48000.0f;

    // Per-sample deltas (computed in configure — Lab 02 Q1).
    float attackDelta_ = 0.0f;
    float decayDelta_ = 0.0f;
    float releaseDelta_ = 0.0f;

    EnvelopeStage stage_ = EnvelopeStage::Idle;
    float level_ = 0.0f;
};
