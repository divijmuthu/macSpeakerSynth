#import <AudioToolbox/AudioToolbox.h>

#include "platform/AudioOutput.h"

#include <iostream>
#include <memory>

namespace {

struct CoreAudioContext {
    AudioComponentInstance unit = nullptr;
    AudioRenderCallback render;
    unsigned channels = 2;
};

OSStatus coreAudioRender(void* inRefCon,
                         AudioUnitRenderActionFlags* /*ioActionFlags*/,
                         const AudioTimeStamp* /*inTimeStamp*/,
                         UInt32 /*inBusNumber*/,
                         UInt32 inNumberFrames,
                         AudioBufferList* ioData) {
    auto* ctx = static_cast<CoreAudioContext*>(inRefCon);
    if (ctx == nullptr || !ctx->render || ioData->mNumberBuffers < 1) {
        return noErr;
    }

    float* out = static_cast<float*>(ioData->mBuffers[0].mData);
    if (out == nullptr) {
        return noErr;
    }

    ctx->render(out, inNumberFrames);
    return noErr;
}

}  // namespace

class CoreAudioOutput final : public AudioOutput {
public:
    bool start(float sampleRate, unsigned channelCount, AudioRenderCallback callback) override {
        if (running_) {
            return true;
        }

        context_ = std::make_unique<CoreAudioContext>();
        context_->render = std::move(callback);
        context_->channels = channelCount;

        AudioComponentDescription desc{};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent component = AudioComponentFindNext(nullptr, &desc);
        if (component == nullptr) {
            std::cerr << "Core Audio: DefaultOutput component not found\n";
            context_.reset();
            return false;
        }

        OSStatus status = AudioComponentInstanceNew(component, &context_->unit);
        if (status != noErr) {
            std::cerr << "Core Audio: AudioComponentInstanceNew failed\n";
            context_.reset();
            return false;
        }

        AudioStreamBasicDescription asbd{};
        asbd.mSampleRate = sampleRate;
        asbd.mFormatID = kAudioFormatLinearPCM;
        asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        asbd.mBitsPerChannel = 32;
        asbd.mChannelsPerFrame = static_cast<UInt32>(channelCount);
        asbd.mFramesPerPacket = 1;
        asbd.mBytesPerFrame = static_cast<UInt32>(channelCount * sizeof(float));
        asbd.mBytesPerPacket = asbd.mBytesPerFrame;

        status = AudioUnitSetProperty(context_->unit,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input,
                                      0,
                                      &asbd,
                                      sizeof(asbd));
        if (status != noErr) {
            std::cerr << "Core Audio: stream format failed\n";
            stop();
            return false;
        }

        AURenderCallbackStruct cb{};
        cb.inputProc = coreAudioRender;
        cb.inputProcRefCon = context_.get();
        status = AudioUnitSetProperty(context_->unit,
                                      kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input,
                                      0,
                                      &cb,
                                      sizeof(cb));
        if (status != noErr) {
            std::cerr << "Core Audio: render callback failed\n";
            stop();
            return false;
        }

        status = AudioUnitInitialize(context_->unit);
        if (status != noErr) {
            std::cerr << "Core Audio: initialize failed\n";
            stop();
            return false;
        }

        status = AudioOutputUnitStart(context_->unit);
        if (status != noErr) {
            std::cerr << "Core Audio: start failed\n";
            stop();
            return false;
        }

        running_ = true;
        return true;
    }

    void stop() override {
        if (context_ != nullptr && context_->unit != nullptr) {
            AudioOutputUnitStop(context_->unit);
            AudioUnitUninitialize(context_->unit);
            AudioComponentInstanceDispose(context_->unit);
            context_->unit = nullptr;
        }
        context_.reset();
        running_ = false;
    }

    const char* backendName() const override { return "Core Audio (macOS HAL)"; }

private:
    std::unique_ptr<CoreAudioContext> context_;
    bool running_ = false;
};

std::unique_ptr<AudioOutput> createCoreAudioOutput() {
    return std::make_unique<CoreAudioOutput>();
}
