#pragma once

#include <array>
#include <cstddef>

// Lab 07: Fixed-size circular delay buffer (master bus effect).
// See learning/lab07/README.md.

class DelayLine {
public:
    static constexpr std::size_t kMaxDelaySamples = 48000;  // 1.0 s @ 48 kHz

    void setDelaySeconds(float seconds, float sampleRate);
    void setFeedback(float feedback);

    // Write input + feedback; return delayed tap (wet source).
    float process(float input);

    void reset();

    std::size_t delaySamples() const { return delaySamples_; }
    float feedback() const { return feedback_; }

private:
    std::array<float, kMaxDelaySamples> buffer_{};
    std::size_t writeIndex_ = 0;
    std::size_t delaySamples_ = 2400;
    float feedback_ = 0.0f;
};
