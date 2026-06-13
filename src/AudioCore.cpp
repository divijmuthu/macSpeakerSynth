#include "AudioCore.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstdio>
#include <iostream>

namespace { 

void audioDataCallback(ma_device* device,
                       void* output,
                       const void* /*input*/,
                       ma_uint32 frameCount) {
    // TODO (Lab 03, Q1): Recover AudioCore from device->pUserData and call renderBlock.
    //
    //   auto* core = static_cast<AudioCore*>(device->pUserData);
    //   core->renderBlock(static_cast<float*>(output), frameCount);
    //
    (void)device;
    (void)output;
    (void)frameCount;
    // map the device's user data to an AudioCore object
    auto* core = static_cast<AudioCore*>(device->pUserData);
    // render a block of audio data to feed the device
    core->renderBlock(static_cast<float*>(output), frameCount);
}

} // namespace

AudioCore::AudioCore() {
    voice_.setFrequency(kDefaultFreqHz, kSampleRate);
    voice_.configureEnvelope(
        /*attack*/ 0.02f,
        /*decay*/ 0.10f,
        /*sustain*/ 0.7f,
        /*release*/ 0.25f,
        kSampleRate);
}

AudioCore::~AudioCore() {
    stop();
}

void AudioCore::renderBlock(float* output, std::uint32_t frameCount) {
    // TODO (Lab 03, Q2): Per-sample loop — the heart of the synth.
    //
    // For each frame i in [0, frameCount):
    //   float s = voice_.nextSample();     // Lab 02 kernel
    //   output[2 * i + 0] = s;             // left
    //   output[2 * i + 1] = s;             // right (mono duplicated)
    //
    (void)output;
    (void)frameCount;
    for (int i = 0; i < frameCount; ++i) {
        const float s = this->voice_.nextSample();
        output[2 * i + 0] = s;
        output[2 * i + 1] = s;
    }
}

bool AudioCore::start() {
    if (running_) {
        return true;
    }
    // create handler for ma device
    device_ = new ma_device;

    // TODO (Lab 03, Q3): Configure the miniaudio playback device.
    //
    // Start from defaults:
    //   ma_device_config config = ma_device_config_init(ma_device_type_playback);
    //
    // Then set:
    //   config.playback.format   = ma_format_f32;
    //   config.playback.channels = 2;
    //   config.sampleRate        = static_cast<ma_uint32>(kSampleRate);
    //   config.dataCallback      = audioDataCallback;
    //   config.pUserData         = this;
    //
    // If ma_device_init(nullptr, &config, device_) != MA_SUCCESS → return false.
    // If ma_device_start(device_) != MA_SUCCESS → ma_device_uninit + return false.
    //
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(kSampleRate);
    config.dataCallback = audioDataCallback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, device_) != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio device" << std::endl;
        return false; 
    }
    if (ma_device_start(device_) != MA_SUCCESS) {
        std::cerr << "Failed to start audio device" << std::endl;
        ma_device_uninit(device_);
        return false;
    }
    // succeeded in initializing and starting the device
    this->running_ = true;
    return true;
}

void AudioCore::stop() {
    if (!running_ || device_ == nullptr) {
        return;
    }
    ma_device_stop(device_);
    ma_device_uninit(device_);
    delete device_;
    device_ = nullptr;
    running_ = false;
}
