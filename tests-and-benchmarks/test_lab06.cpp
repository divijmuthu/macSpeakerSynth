// Lab 06 tests — TPT state-variable filter + multimode.
// Build: cmake --build build && ./build/test_lab06

#include "AudioCore.h"
#include "ControlParser.h"
#include "DSP_utilities/StateVariableFilter.h"
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

float measureSinePeak(StateVariableFilter& filter, FilterMode mode, float freqHz, int sampleCount) {
    filter.reset();
    filter.setMode(mode);
    float peak = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        const float input = std::sin(2.0f * kPi * freqHz * t);
        const float output = filter.process(input);
        peak = std::fmax(peak, std::fabs(output));
    }
    return peak;
}

void test_svf_low_pass_attenuates_highs() {
    StateVariableFilter svf;
    svf.setCutoff(500.0f, kSampleRate);

    const float lowPeak = measureSinePeak(svf, FilterMode::LowPass, 100.0f, kSampleRate / 2);
    const float highPeak = measureSinePeak(svf, FilterMode::LowPass, 8000.0f, kSampleRate / 2);

    check(lowPeak > highPeak * 2.0f, "SVF LP: 100 Hz louder than 8 kHz at fc=500");
}

void test_svf_high_pass_attenuates_lows() {
    StateVariableFilter svf;
    svf.setCutoff(800.0f, kSampleRate);

    const float lowPeak = measureSinePeak(svf, FilterMode::HighPass, 80.0f, kSampleRate / 2);
    const float highPeak = measureSinePeak(svf, FilterMode::HighPass, 4000.0f, kSampleRate / 2);

    check(highPeak > lowPeak * 2.0f, "SVF HP: 4 kHz louder than 80 Hz at fc=800");
}

void test_svf_band_pass_passes_band() {
    StateVariableFilter svf;
    svf.setCutoff(1000.0f, kSampleRate);

    const float atPeak = measureSinePeak(svf, FilterMode::BandPass, 1000.0f, kSampleRate / 2);
    const float offPeak = measureSinePeak(svf, FilterMode::BandPass, 100.0f, kSampleRate / 2);

    check(atPeak > offPeak * 2.0f, "SVF BP: 1 kHz louder than 100 Hz at fc=1 kHz");
}

void test_svf_multimode_same_tick() {
    StateVariableFilter svf;
    svf.setCutoff(1000.0f, kSampleRate);
    svf.setMode(FilterMode::LowPass);
    (void)svf.process(1.0f);
    const float lp = svf.lastLow();
    const float bp = svf.lastBand();
    const float hp = svf.lastHigh();

    check(std::fabs(lp - bp) > 1e-4f || std::fabs(lp - hp) > 1e-4f,
          "one process() computes distinct LP/BP/HP taps");
}

void test_parse_filter_mode() {
    ControlMessage msg{};
    check(parseControlMessage("MODE,2", msg), "parse MODE");
    check(msg.type == ControlType::FilterMode && static_cast<int>(msg.frequencyHz + 0.5f) == 2,
          "MODE value 2 = BandPass");
}

void test_voice_high_pass_mode() {
    Voice voice;
    voice.setFrequency(2000.0f, kSampleRate);  // above HP cutoff
    voice.configureEnvelope(0.001f, 0.05f, 0.8f, 0.1f, kSampleRate);
    voice.setCutoff(800.0f, kSampleRate);
    voice.setFilterMode(FilterMode::HighPass);
    voice.noteOn();

    std::vector<float> buffer(4096, 0.0f);
    for (int i = 0; i < 8; ++i) {
        for (std::size_t s = 0; s < buffer.size(); ++s) {
            buffer[s] = voice.nextSample();
        }
    }

    float peak = 0.0f;
    for (float x : buffer) {
        peak = std::fmax(peak, std::fabs(x));
    }
    check(peak > 0.05f, "Voice HP mode produces non-silent output");
}

void test_drain_filter_mode() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 440.0f});
    queue.push({ControlType::FilterMode, 1.0f});  // HP
    core.drainControlQueue();

    check(core.voice().filter().mode() == FilterMode::HighPass,
          "drain MODE,1 sets HighPass on voice filter");
}

}  // namespace

int main() {
    std::printf("Lab 06 — State-variable filter tests\n");
    std::printf("====================================\n");

    test_svf_low_pass_attenuates_highs();
    test_svf_high_pass_attenuates_lows();
    test_svf_band_pass_passes_band();
    test_svf_multimode_same_tick();
    test_parse_filter_mode();
    test_voice_high_pass_mode();
    test_drain_filter_mode();

    std::printf("====================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in StateVariableFilter.cpp — see learning/lab06/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 06 tests done. Try: m 0 (LP), m 1 (HP), m 2 (BP) in Python UI\n");
    return EXIT_SUCCESS;
}
