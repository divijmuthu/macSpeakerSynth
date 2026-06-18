// Lab 07 tests — delay line, soft clip, master bus.
// Build: cmake --build build && ./build/test_lab07

#include "AudioCore.h"
#include "ControlParser.h"
#include "DSP_utilities/DelayLine.h"
#include "DSP_utilities/SoftClip.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

constexpr float kSampleRate = 48000.0f;

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::printf("ok     %s\n", name);
        ++g_passed;
    } else {
        std::printf("not ok %s\n", name);
        ++g_failed;
    }
}

void test_delay_impulse_at_tap() {
    DelayLine delay;
    delay.setDelaySeconds(0.01f, kSampleRate);  // 10 ms = 480 samples
    delay.setFeedback(0.0f);

    delay.process(1.0f);
    float peak = 0.0f;
    for (int i = 0; i < static_cast<int>(0.02f * kSampleRate); ++i) {
        peak = std::fmax(peak, std::fabs(delay.process(0.0f)));
    }
    check(peak > 0.5f, "delay tap repeats impulse after delay time");
}

void test_delay_feedback_sustains_tail() {
    DelayLine delay;
    delay.setDelaySeconds(0.005f, kSampleRate);
    delay.setFeedback(0.6f);

    delay.process(1.0f);
    float latePeak = 0.0f;
    for (int i = 0; i < static_cast<int>(0.05f * kSampleRate); ++i) {
        latePeak = std::fmax(latePeak, std::fabs(delay.process(0.0f)));
    }
    check(latePeak > 0.05f, "feedback keeps echo tail alive");
}

void test_soft_clip_limits_peak() {
    SoftClip clip;
    clip.setDrive(3.0f);

    const float hard = clip.process(10.0f);
    check(std::fabs(hard) < 1.05f, "soft clip bounds large input");
    check(std::fabs(hard) > 0.5f, "soft clip still passes signal");
}

void test_master_effects_wet_mix() {
    AudioCore core(nullptr);
    core.voice().setFrequency(440.0f, kSampleRate);
    core.voice().configureEnvelope(0.001f, 0.05f, 0.8f, 0.1f, kSampleRate);
    core.voice().noteOn();

    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore wired(&queue);
    wired.voice().setFrequency(440.0f, kSampleRate);
    wired.voice().configureEnvelope(0.001f, 0.05f, 0.8f, 0.1f, kSampleRate);
    queue.push({ControlType::DelayTime, 0.05f});
    queue.push({ControlType::DelayFeedback, 0.5f});
    queue.push({ControlType::DelayWet, 0.8f});
    wired.drainControlQueue();
    wired.voice().noteOn();

    std::vector<float> buffer(8192, 0.0f);
    for (int i = 0; i < 16; ++i) {
        wired.renderBlock(buffer.data(), 512);
    }

    float peak = 0.0f;
    for (float s : buffer) {
        peak = std::fmax(peak, std::fabs(s));
    }
    check(peak > 0.1f, "master chain produces output with delay engaged");
}

void test_parse_delay_messages() {
    ControlMessage msg{};
    check(parseControlMessage("DELAY,0.250000", msg), "parse DELAY");
    check(msg.type == ControlType::DelayTime && std::fabs(msg.frequencyHz - 0.25f) < 1e-4f,
          "DELAY value");
    check(parseControlMessage("FEEDBACK,0.450000", msg), "parse FEEDBACK");
    check(parseControlMessage("WET,0.350000", msg), "parse WET");
    check(parseControlMessage("DRIVE,1.500000", msg), "parse DRIVE");
}

}  // namespace

int main() {
    std::printf("Lab 07 — Delay + soft clip tests\n");
    std::printf("=================================\n");

    test_delay_impulse_at_tap();
    test_delay_feedback_sustains_tail();
    test_soft_clip_limits_peak();
    test_master_effects_wet_mix();
    test_parse_delay_messages();

    std::printf("=================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs — see learning/lab07/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 07 tests done. GUI: Apply effects panel + piano keyboard\n");
    return EXIT_SUCCESS;
}
