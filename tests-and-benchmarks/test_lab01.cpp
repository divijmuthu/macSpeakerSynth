// Lab 01 tests — CS61A-style ok / not ok output.
// Build: cmake --build build && ./build/test_lab01

#include "DSP_utilities/Oscillator.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kFreq = 440.0f;
constexpr float kTwoPi = 2.0f * static_cast<float>(M_PI);

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

bool near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

void test_phase_increment() {
    Oscillator osc;
    osc.setFrequency(kFreq, kSampleRate);
    const float expected = kTwoPi * kFreq / kSampleRate;
    check(near(osc.phaseIncrement(), expected), "phase increment matches 2*pi*f/fs");
}

void test_first_sample_is_zero() {
    Oscillator osc;
    osc.setFrequency(kFreq, kSampleRate);
    osc.resetPhase();
    check(near(osc.nextSample(), 0.0f), "first sample at phase 0 is sin(0) == 0");
}

void test_peak_near_quarter_cycle() {
    Oscillator osc;
    osc.setFrequency(kFreq, kSampleRate);
    osc.resetPhase();

    const int samplesPerCycle = static_cast<int>(kSampleRate / kFreq);
    const int quarter = samplesPerCycle / 4;

    float last = 0.0f;
    for (int i = 0; i < quarter; ++i) {
        last = osc.nextSample();
    }
    check(last > 0.95f, "amplitude near +1 after ~quarter cycle");
}

void test_many_samples_stay_bounded() {
    Oscillator osc;
    osc.setFrequency(kFreq, kSampleRate);
    osc.resetPhase();

    bool bounded = true;
    float peak = 0.0f;
    for (int i = 0; i < static_cast<int>(kSampleRate); ++i) {
        const float s = osc.nextSample();
        peak = std::fmax(peak, std::fabs(s));
        if (s < -1.01f || s > 1.01f) {
            bounded = false;
            break;
        }
    }
    check(bounded && peak > 0.9f,
          "48000 samples stay within [-1, 1] and actually oscillate (phase wrap works)");
}

void test_phase_wraps_not_drifts() {
    Oscillator osc;
    osc.setFrequency(kFreq, kSampleRate);
    osc.resetPhase();

    for (int i = 0; i < static_cast<int>(kSampleRate * 2); ++i) {
        (void)osc.nextSample();
    }
    const bool inRange = osc.phase() >= 0.0f && osc.phase() < kTwoPi;
    const bool moved = osc.phase() > 0.0f || osc.phaseIncrement() > 0.0f;
    check(inRange && moved, "phase stays in [0, 2*pi) and advances after 2 seconds");
}

}  // namespace

int main() {
    std::printf("Lab 01 — Oscillator tests\n");
    std::printf("=========================\n");

    test_phase_increment();
    test_first_sample_is_zero();
    test_peak_near_quarter_cycle();
    test_many_samples_stay_bounded();
    test_phase_wraps_not_drifts();

    std::printf("=========================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_passed + g_failed - g_passed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in src/DSP_utilities/Oscillator.cpp — see learning/lab01/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 01 complete. Next: learning/lab02/README.md\n");
    return EXIT_SUCCESS;
}
