#include "AudioCore.h"

#include "ControlMessage.h"
#include "DSP_utilities/StateVariableFilter.h"
#include "DSP_utilities/Waveform.h"
#include "platform/AudioOutput.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <iostream>

AudioCore::AudioCore(LockFreeQueue<kControlQueueCapacity>* controlQueue)
    : controlQueue_(controlQueue) {
    for (auto& slot : voices_) {
        configureVoiceDefaults(slot.voice);
    }
    delayLine_.setDelaySeconds(0.25f, kSampleRate);
    delayLine_.setFeedback(0.0f);
    delayWet_ = 0.0f;
    softClip_.setDrive(1.0f);
}

void AudioCore::configureVoiceDefaults(Voice& voice) {
    voice.setFrequency(kDefaultFreqHz, kSampleRate);
    voice.configureEnvelope(
        /*attack*/ 0.02f,
        /*decay*/ 0.10f,
        /*sustain*/ 0.7f,
        /*release*/ 0.08f,
        kSampleRate);
    voice.setCutoff(12000.0f, kSampleRate);
    voice.setFilterMode(FilterMode::LowPass);
}

std::size_t AudioCore::activeVoiceCount() const {
    std::size_t count = 0;
    for (const auto& slot : voices_) {
        if (slot.voice.isActive()) {
            ++count;
        }
    }
    return count;
}

std::size_t AudioCore::allocateVoice(float frequencyHz) {
    // TODO (Lab 08, Q1): Voice allocation on NOTE_ON.
    // 1. If a slot already plays this frequency (within ~0.5 Hz), retrigger it.
    // 2. Else pick the first idle slot (envelope not active).
    // 3. If all busy, steal the slot with the smallest allocGeneration.
    // Return slot index.
    // see if frequency is already being played
    for (auto& slot : voices_) {
        // check if float abs val diff is less than 0.5, if so retrigger that slot
        if (slot.voice.isActive() && std::fabs(slot.frequencyHz - frequencyHz) < 0.5f) {
            slot.frequencyHz = frequencyHz;
            slot.allocGeneration = ++allocCounter_;
            slot.voice.setFrequency(frequencyHz, kSampleRate);
            slot.voice.noteOn();
            return std::distance(voices_.begin(), &slot);
        }
    }
    // if no slot is playing this frequency, pick the first idle slot
    for (auto& slot : voices_) {
        if (!slot.voice.isActive()) {
            slot.frequencyHz = frequencyHz;
            slot.allocGeneration = ++allocCounter_;
            slot.voice.setFrequency(frequencyHz, kSampleRate);
            slot.voice.noteOn();
            return std::distance(voices_.begin(), &slot);
        }
    }
    // if all slots are busy, steal the slot with the smallest allocGeneration
    auto minSlot = std::min_element(voices_.begin(), voices_.end(), [](const auto& a, const auto& b) {
        return a.allocGeneration < b.allocGeneration;
    });
    minSlot->frequencyHz = frequencyHz;
    minSlot->allocGeneration = ++allocCounter_;
    minSlot->voice.setFrequency(frequencyHz, kSampleRate);
    minSlot->voice.noteOn();
    return std::distance(voices_.begin(), minSlot);
}

void AudioCore::releaseVoice(float frequencyHz) {
    // TODO (Lab 08, Q2): NOTE_OFF targets one voice (or all).
    //
    // frequencyHz == 0  → noteOff() every active slot
    // else              → find active slot with matching frequencyHz, noteOff() it
    //
    if (std::fabs(frequencyHz) < 0.0001f) {
        for (auto& slot : voices_) {
            if (slot.voice.isActive()) {
                slot.voice.noteOff();
            }
        }
        return;
    }

    // Go through active slots and release the matching pitch.
    for (auto& slot : voices_) {
        if (slot.voice.isActive() && std::fabs(slot.frequencyHz - frequencyHz) < 0.5f) {
            slot.voice.noteOff();
            return;
        }
    }
}

float AudioCore::mixActiveVoices() {
    float mix = 0.0f;
    std::size_t activeCount = 0;
    for (auto& slot : voices_) {
        if (slot.voice.isActive()) {
            mix += slot.voice.nextSample();
            ++activeCount;
        }
    }

    if (activeCount == 0) {
        mixGainCurrent_ = 1.0f;
        return 0.0f;
    }

    // Polyphonic headroom strategy:
    // - 1 voice: unity gain
    // - N voices: conservative 1/N normalization (no chord overage clipping)
    const float targetGain =
        (activeCount == 1) ? 1.0f : (0.9f / static_cast<float>(activeCount));

    // Smooth gain transitions so 1→2 notes (or release) doesn't click/pop.
    constexpr float kGainSlew = 0.002f;  // ~10 ms at 48 kHz
    mixGainCurrent_ += (targetGain - mixGainCurrent_) * kGainSlew;
    mix *= mixGainCurrent_;

    // Safety clamp for DAC range.
    return std::clamp(mix, -0.98f, 0.98f);
}

void AudioCore::applyGlobalToAllVoices(const ControlMessage& message) {
    switch (message.type) {
        case ControlType::Cutoff:
            for (auto& slot : voices_) {
                slot.voice.setCutoff(message.frequencyHz, kSampleRate);
            }
            break;
        case ControlType::FilterMode: {
            const int modeIndex = static_cast<int>(message.frequencyHz + 0.5f);
            if (modeIndex >= 0 && modeIndex <= 2) {
                const auto mode = static_cast<FilterMode>(modeIndex);
                for (auto& slot : voices_) {
                    slot.voice.setFilterMode(mode);
                }
            }
            break;
        }
        case ControlType::Waveform: {
            const int wf = static_cast<int>(message.frequencyHz + 0.5f);
            if (wf >= 0 && wf <= 3) {
                const auto waveform = static_cast<Waveform>(wf);
                for (auto& slot : voices_) {
                    slot.voice.setWaveform(waveform);
                }
            }
            break;
        }
        default:
            break;
    }
}

void AudioCore::drainControlQueue() {
    if (controlQueue_ == nullptr) {
        return;
    }

    ControlMessage message;
    while (controlQueue_->pop(message)) {
        switch (message.type) {
            case ControlType::NoteOn:
                allocateVoice(message.frequencyHz);
                break;
            case ControlType::NoteOff:
                releaseVoice(message.frequencyHz);
                break;
            case ControlType::Cutoff:
            case ControlType::FilterMode:
            case ControlType::Waveform:
                applyGlobalToAllVoices(message);
                break;
            case ControlType::DelayTime:
                delayLine_.setDelaySeconds(message.frequencyHz, kSampleRate);
                break;
            case ControlType::DelayFeedback:
                delayLine_.setFeedback(message.frequencyHz);
                break;
            case ControlType::DelayWet:
                delayWet_ = std::clamp(message.frequencyHz, 0.0f, 1.0f);
                if (delayWet_ <= 0.0001f) {
                    delayLine_.reset();
                }
                break;
            case ControlType::Drive:
                softClip_.setDrive(message.frequencyHz);
                break;
        }
    }
}

float AudioCore::processMasterEffects(float dry) {
    float mixed = dry;
    if (delayWet_ > 0.0001f) {
        const float wet = delayLine_.process(dry);
        mixed = dry * (1.0f - delayWet_) + wet * delayWet_;
    }
    if (softClip_.drive() > 1.0f + 1e-6f) {
        return softClip_.process(mixed);
    }
    return mixed;
}

AudioCore::~AudioCore() {
    stop();
}

void AudioCore::renderBlock(float* output, std::uint32_t frameCount) {
    drainControlQueue();

    for (std::uint32_t i = 0; i < frameCount; ++i) {
        const float dry = mixActiveVoices();
        const float s = processMasterEffects(dry);

        float scopeEnv = 0.0f;
        for (const auto& slot : voices_) {
            scopeEnv = std::fmax(scopeEnv, slot.voice.envelope().level());
        }
        pushScopeSample(s, scopeEnv);

        output[2 * i + 0] = s;
        output[2 * i + 1] = s;
    }
}

void AudioCore::pushScopeSample(float output, float envelope) {
    const std::size_t idx = scopeWrite_.load(std::memory_order_relaxed);
    scopeOutput_[idx] = output;
    scopeEnvelope_[idx] = envelope;
    scopeWrite_.store((idx + 1) % kScopeSize, std::memory_order_release);
}

void AudioCore::copyScopeSnapshot(float* output, float* envelope) const {
    const std::size_t w = scopeWrite_.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < kScopeSize; ++i) {
        const std::size_t idx = (w + i) % kScopeSize;
        output[i] = scopeOutput_[idx];
        envelope[i] = scopeEnvelope_[idx];
    }
}

const char* AudioCore::audioBackendName() const {
    if (audioOutput_ != nullptr) {
        return audioOutput_->backendName();
    }
    return "none";
}

bool AudioCore::start() {
    if (running_) {
        return true;
    }

    audioOutput_ = AudioOutput::createDefault();
    if (audioOutput_ == nullptr) {
        std::cerr << "Failed to create audio output backend\n";
        return false;
    }

    const bool ok = audioOutput_->start(
        kSampleRate,
        2,
        [this](float* buffer, std::uint32_t frameCount) { renderBlock(buffer, frameCount); });

    if (!ok) {
        audioOutput_.reset();
        return false;
    }

    running_ = true;
    std::printf("Audio backend: %s  (%zu voices)\n", audioBackendName(), kMaxVoices);
    return true;
}

void AudioCore::stop() {
    if (!running_ || audioOutput_ == nullptr) {
        return;
    }
    audioOutput_->stop();
    audioOutput_.reset();
    running_ = false;
}
