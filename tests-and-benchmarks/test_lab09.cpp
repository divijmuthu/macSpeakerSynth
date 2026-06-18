// Lab 09 — waveform shapes + Lab 10 SIMD batch hook.
// Build: cmake --build build && ./build/test_lab09

#include "ControlParser.h"
#include "DSP_utilities/Oscillator.h"
#include "DSP_utilities/SimdOscillator.h"
#include "DSP_utilities/Waveform.h"

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

float measurePeak(Waveform wf, int count = 4096) {
    Oscillator osc;
    osc.setFrequency(440.0f, kSampleRate);
    osc.setWaveform(wf);
    float peak = 0.0f;
    for (int i = 0; i < count; ++i) {
        peak = std::fmax(peak, std::fabs(osc.nextSample()));
    }
    return peak;
}

void test_saw_has_harmonics() {
    Oscillator osc;
    osc.setFrequency(100.0f, kSampleRate);
    osc.setWaveform(Waveform::Saw);
    float peak = 0.0f;
    for (int i = 0; i < 4800; ++i) {
        peak = std::fmax(peak, std::fabs(osc.nextSample()));
    }
    check(peak > 0.5f, "saw produces audible output");
}

void test_square_alternates_sign() {
    Oscillator osc;
    osc.setFrequency(200.0f, kSampleRate);
    osc.setWaveform(Waveform::Square);
    bool pos = false;
    bool neg = false;
    for (int i = 0; i < 2000; ++i) {
        const float s = osc.nextSample();
        if (s > 0.5f) {
            pos = true;
        }
        if (s < -0.5f) {
            neg = true;
        }
    }
    check(pos && neg, "square hits +1 and -1");
}

void test_triangle_peak() {
    check(measurePeak(Waveform::Triangle) > 0.5f, "triangle audible");
}

void test_batch_matches_scalar() {
    Oscillator scalar;
    Oscillator batch;
    scalar.setFrequency(440.0f, kSampleRate);
    batch.setFrequency(440.0f, kSampleRate);
    scalar.setWaveform(Waveform::Saw);
    batch.setWaveform(Waveform::Saw);

    std::vector<float> buf(64, 0.0f);
    oscillatorRenderBatch(batch, Waveform::Saw, buf.data(), buf.size());

    float maxDiff = 0.0f;
    for (std::size_t i = 0; i < buf.size(); ++i) {
        const float expected = scalar.nextSample();
        maxDiff = std::fmax(maxDiff, std::fabs(buf[i] - expected));
    }
    check(maxDiff < 1e-4f, "SIMD batch matches scalar saw");
}

void test_parse_waveform() {
    ControlMessage msg{};
    check(parseControlMessage("WAVEFORM,2", msg), "parse WAVEFORM");
    check(msg.type == ControlType::Waveform && static_cast<int>(msg.frequencyHz + 0.5f) == 2,
          "WAVEFORM value square");
}

}  // namespace

int main() {
    std::printf("Lab 09 — Waveforms + SIMD batch\n");
    std::printf("================================\n");

    test_saw_has_harmonics();
    test_square_alternates_sign();
    test_triangle_peak();
    test_batch_matches_scalar();
    test_parse_waveform();

    std::printf("================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
