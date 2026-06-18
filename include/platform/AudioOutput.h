#pragma once

#include <cstdint>
#include <functional>
#include <memory>

// Platform audio device wrapper (Lab 11).
// AudioCore supplies renderBlock; backend pushes samples to the DAC.

using AudioRenderCallback = std::function<void(float* stereoInterleaved, std::uint32_t frameCount)>;

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    virtual bool start(float sampleRate, unsigned channelCount, AudioRenderCallback callback) = 0;
    virtual void stop() = 0;
    virtual const char* backendName() const = 0;

    // macOS: Core Audio HAL when RACE_USE_COREAUDIO=ON, else miniaudio.
    static std::unique_ptr<AudioOutput> createDefault();
};
