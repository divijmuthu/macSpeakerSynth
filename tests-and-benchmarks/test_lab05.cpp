// Lab 05 tests — Biquad LPF + Voice integration (no speakers).
// Build: cmake --build build && ./build/test_lab05

#include "AudioCore.h"
#include "ControlParser.h"
#include "DSP_utilities/Biquad.h"
#include "DSP_utilities/Voice.h"
#include "LockFreeQueue.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kPi = 3.14159265358979323846f;

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

float measureSinePeak(Biquad& filter, float freqHz, int sampleCount) {
    filter.reset();
    float peak = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        const float input = std::sin(2.0f * kPi * freqHz * t);
        const float output = filter.process(input);
        peak = std::fmax(peak, std::fabs(output));
    }
    return peak;
}

void test_biquad_dc_passes() {
    Biquad filter;
    filter.setLowPass(1000.0f, kSampleRate);
    filter.reset();

    float output = 0.0f;
    for (int i = 0; i < 500; ++i) {
        output = filter.process(0.5f);
    }
    check(std::fabs(output - 0.5f) < 0.05f, "low-pass passes DC / steady input");
}

void test_biquad_attenuates_highs() {
    Biquad filter;
    filter.setLowPass(500.0f, kSampleRate);

    const float lowPeak = measureSinePeak(filter, 100.0f, kSampleRate / 2);
    const float highPeak = measureSinePeak(filter, 8000.0f, kSampleRate / 2);

    check(lowPeak > highPeak * 2.0f, "500 Hz LPF: 100 Hz louder than 8 kHz");
}

void test_biquad_open_vs_closed() {
    Biquad wide;
    wide.setLowPass(16000.0f, kSampleRate);
    Biquad narrow;
    narrow.setLowPass(200.0f, kSampleRate);

    const float widePeak = measureSinePeak(wide, 440.0f, kSampleRate / 4);
    const float narrowPeak = measureSinePeak(narrow, 440.0f, kSampleRate / 4);

    check(widePeak > narrowPeak * 1.5f, "wide cutoff passes 440 Hz more than narrow");
}

void test_parse_cutoff() {
    ControlMessage msg{};
    check(parseControlMessage("CUTOFF,1200.0", msg), "parse CUTOFF");
    check(msg.type == ControlType::Cutoff && std::fabs(msg.frequencyHz - 1200.0f) < 1e-3f,
          "CUTOFF value extracted");
}

void test_voice_filter_reduces_brightness() {
    Voice voice;
    voice.setFrequency(440.0f, kSampleRate);
    voice.configureEnvelope(0.001f, 0.05f, 0.8f, 0.1f, kSampleRate);
    voice.setCutoff(16000.0f, kSampleRate);
    voice.noteOn();

    std::vector<float> bright(4096, 0.0f);
    for (int i = 0; i < 8; ++i) {
        for (std::size_t s = 0; s < bright.size(); ++s) {
            bright[s] = voice.nextSample();
        }
    }
    float brightPeak = 0.0f;
    for (float x : bright) {
        brightPeak = std::fmax(brightPeak, std::fabs(x));
    }

    voice.setCutoff(150.0f, kSampleRate);
    voice.noteOn();

    std::vector<float> dull(4096, 0.0f);
    for (int i = 0; i < 8; ++i) {
        for (std::size_t s = 0; s < dull.size(); ++s) {
            dull[s] = voice.nextSample();
        }
    }
    float dullPeak = 0.0f;
    for (float x : dull) {
        dullPeak = std::fmax(dullPeak, std::fabs(x));
    }

    check(brightPeak > dullPeak * 1.2f, "Voice: lower cutoff quiets 440 Hz tone");
}

void test_drain_cutoff_message() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 440.0f});
    queue.push({ControlType::Cutoff, 16000.0f});
    core.drainControlQueue();

    std::vector<float> bright(4096, 0.0f);
    for (int i = 0; i < 8; ++i) {
        core.renderBlock(bright.data(), 512);
    }
    float brightPeak = 0.0f;
    for (float s : bright) {
        brightPeak = std::fmax(brightPeak, std::fabs(s));
    }

    queue.push({ControlType::Cutoff, 150.0f});
    core.drainControlQueue();

    std::vector<float> dull(4096, 0.0f);
    for (int i = 0; i < 8; ++i) {
        core.renderBlock(dull.data(), 512);
    }
    float dullPeak = 0.0f;
    for (float s : dull) {
        dullPeak = std::fmax(dullPeak, std::fabs(s));
    }

    check(brightPeak > 0.2f && dullPeak < brightPeak * 0.5f,
          "drain CUTOFF lowers level vs wide open (150 vs 16k Hz)");
}

}  // namespace

int main() {
    std::printf("Lab 05 — Biquad + Voice tests\n");
    std::printf("==============================\n");

    test_biquad_dc_passes();
    test_biquad_attenuates_highs();
    test_biquad_open_vs_closed();
    test_parse_cutoff();
    test_voice_filter_reduces_brightness();
    test_drain_cutoff_message();

    std::printf("==============================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in Biquad.cpp and Voice.cpp — see learning/lab05/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 05 tests done. Run race_synth + Python; try: c 500 / c 8000\n");
    return EXIT_SUCCESS;
}
