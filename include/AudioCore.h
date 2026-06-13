#pragma once

#include "DSP_utilities/Voice.h"

#include <cstdint>

struct ma_device; // get a miniaudio device to interact with the hardware

// Lab 03: Owns a Voice and drives it from the miniaudio real-time callback.
//
// renderBlock() is the inner loop you will reuse in Labs 04–06.
// See learning/lab03/README.md.

class AudioCore {
public:
    static constexpr float kSampleRate = 48000.0f;
    static constexpr float kDefaultFreqHz = 440.0f;

    AudioCore(); // default constructor
    ~AudioCore(); // default destructor

    AudioCore(const AudioCore&) = delete;
    AudioCore& operator=(const AudioCore&) = delete;

    bool start();
    void stop();

    Voice& voice() { return voice_; }
    const Voice& voice() const { return voice_; }

    // Fill one interleaved stereo buffer (L, R, L, R, …).
    // Called from the miniaudio callback — must be real-time safe.
    void renderBlock(float* output, std::uint32_t frameCount);

private:
    Voice voice_;
    ma_device* device_ = nullptr;
    bool running_ = false;
};
