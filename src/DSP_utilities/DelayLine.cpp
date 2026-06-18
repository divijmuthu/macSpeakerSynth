#include "DSP_utilities/DelayLine.h"

#include <algorithm>
#include <cmath>

void DelayLine::setDelaySeconds(float seconds, float sampleRate) {
    const float clamped = std::clamp(seconds, 0.001f, 1.0f);
    delaySamples_ = static_cast<std::size_t>(clamped * sampleRate);
    if (delaySamples_ < 1) {
        delaySamples_ = 1;
    }
    if (delaySamples_ >= kMaxDelaySamples) {
        delaySamples_ = kMaxDelaySamples - 1;
    }
}

void DelayLine::setFeedback(float feedback) {
    feedback_ = std::clamp(feedback, 0.0f, 0.95f);
}

float DelayLine::process(float input) {
    // TODO (Lab 07, Q1): Circular buffer read/write with feedback.
    // read the index of the delayed sample, add max to block negative index 
    std::size_t readIndex = (writeIndex_ + kMaxDelaySamples - delaySamples_) % kMaxDelaySamples;
    // read delayed sample, use it to compute value to write 
    float delayed = buffer_[readIndex];
    float toWrite = input + delayed * feedback_;
    buffer_[writeIndex_] = toWrite;
    // advance write index to compute next sample with wraparound
    writeIndex_ = (writeIndex_ + 1) % kMaxDelaySamples;
    return delayed;
}

void DelayLine::reset() {
    buffer_.fill(0.0f);
    writeIndex_ = 0;
}
