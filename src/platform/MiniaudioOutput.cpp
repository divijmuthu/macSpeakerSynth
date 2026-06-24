#include "platform/AudioOutput.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

struct MiniaudioContext {
    ma_device device{};
    AudioRenderCallback render;
};

void miniaudioCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount) {
    auto* ctx = static_cast<MiniaudioContext*>(device->pUserData);
    if (ctx != nullptr && ctx->render) {
        ctx->render(static_cast<float*>(output), frameCount);
    }
}

}  // namespace

class MiniaudioOutput final : public AudioOutput {
public:
    bool start(float sampleRate, unsigned channelCount, AudioRenderCallback callback) override {
        if (running_) {
            return true;
        }
        context_.render = std::move(callback);

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = static_cast<ma_uint32>(channelCount);
        config.sampleRate = static_cast<ma_uint32>(sampleRate);
        config.dataCallback = miniaudioCallback;
        config.pUserData = &context_;

        if (ma_device_init(nullptr, &config, &context_.device) != MA_SUCCESS) {
            std::cerr << "miniaudio: device init failed\n";
            return false;
        }
        if (ma_device_start(&context_.device) != MA_SUCCESS) {
            std::cerr << "miniaudio: device start failed\n";
            ma_device_uninit(&context_.device);
            return false;
        }
        running_ = true;
        return true;
    }

    void stop() override {
        if (!running_) {
            return;
        }
        ma_device_stop(&context_.device);
        ma_device_uninit(&context_.device);
        running_ = false;
    }

    const char* backendName() const override { return "miniaudio (cross-platform)"; }

private:
    MiniaudioContext context_{};
    bool running_ = false;
};

#if defined(RACE_USE_COREAUDIO) && defined(__APPLE__)
std::unique_ptr<AudioOutput> AudioOutput::createDefault() {
    extern std::unique_ptr<AudioOutput> createCoreAudioOutput();
    const char* preferred = std::getenv("RACE_AUDIO_BACKEND");
    if (preferred != nullptr) {
        const std::string_view choice(preferred);
        if (choice == "miniaudio") {
            return std::make_unique<MiniaudioOutput>();
        }
        if (choice == "coreaudio") {
            return createCoreAudioOutput();
        }
        std::cerr << "Unknown RACE_AUDIO_BACKEND=" << preferred
                  << " (expected coreaudio|miniaudio), using Core Audio\n";
    }
    return createCoreAudioOutput();
}
#else
std::unique_ptr<AudioOutput> AudioOutput::createDefault() {
    return std::make_unique<MiniaudioOutput>();
}
#endif
