#pragma once

#include "ControlMessage.h"
#include "DSP_utilities/DelayLine.h"
#include "DSP_utilities/SoftClip.h"
#include "DSP_utilities/Voice.h"
#include "LockFreeQueue.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

class AudioOutput;

// Lab 03–11: Real-time engine. Lab 08: polyphonic voice pool.

class AudioCore {
public:
    static constexpr float kSampleRate = 48000.0f;
    static constexpr float kDefaultFreqHz = 440.0f;
    static constexpr std::size_t kControlQueueCapacity = 256;
    static constexpr std::size_t kScopeSize = 256;
    static constexpr std::size_t kMaxVoices = 8;

    struct VoiceSlot {
        Voice voice;
        float frequencyHz = 0.0f;
        std::uint32_t allocGeneration = 0;
    };

    explicit AudioCore(LockFreeQueue<kControlQueueCapacity>* controlQueue = nullptr);

    AudioCore(const AudioCore&) = delete;
    AudioCore& operator=(const AudioCore&) = delete;

    ~AudioCore();

    bool start();
    void stop();

    // Back-compat for Labs 03–07 tests — voice slot 0.
    Voice& voice() { return voices_[0].voice; }
    const Voice& voice() const { return voices_[0].voice; }

    Voice& voiceAt(std::size_t index) { return voices_[index].voice; }
    const Voice& voiceAt(std::size_t index) const { return voices_[index].voice; }
    const VoiceSlot& voiceSlot(std::size_t index) const { return voices_[index]; }

    std::size_t activeVoiceCount() const;

    void drainControlQueue();
    void renderBlock(float* output, std::uint32_t frameCount);

    float processMasterEffects(float dry);

    const DelayLine& delayLine() const { return delayLine_; }
    const SoftClip& softClip() const { return softClip_; }

    void copyScopeSnapshot(float* output, float* envelope) const;

    const char* audioBackendName() const;

private:
    void configureVoiceDefaults(Voice& voice);
    void applyGlobalToAllVoices(const ControlMessage& message);

    // Lab 08 TODOs — see learning/lab08/README.md
    std::size_t allocateVoice(float frequencyHz);
    void releaseVoice(float frequencyHz);
    float mixActiveVoices();

    void pushScopeSample(float output, float envelope);

    std::array<VoiceSlot, kMaxVoices> voices_{};
    std::uint32_t allocCounter_ = 0;
    float mixGainCurrent_ = 1.0f;

    DelayLine delayLine_;
    SoftClip softClip_;
    float delayWet_ = 0.35f;
    float masterGain_ = 0.8f;
    bool simdMode_ = false;

    LockFreeQueue<kControlQueueCapacity>* controlQueue_ = nullptr;
    std::unique_ptr<AudioOutput> audioOutput_;
    bool running_ = false;

    float scopeOutput_[kScopeSize]{};
    float scopeEnvelope_[kScopeSize]{};
    std::atomic<std::size_t> scopeWrite_{0};
};
