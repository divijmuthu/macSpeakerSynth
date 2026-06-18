// Lab 10 — scalar vs NEON batch oscillator throughput.
// Build: cmake --build build && ./build/benchmark_neon

#include "DSP_utilities/Oscillator.h"
#include "DSP_utilities/SimdOscillator.h"
#include "DSP_utilities/Waveform.h"

#include <chrono>
#include <cstdio>
#include <vector>

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr int kIterations = 2000;
constexpr std::size_t kBatch = 256;

double benchScalar() {
    Oscillator osc;
    osc.setFrequency(440.0f, kSampleRate);
    osc.setWaveform(Waveform::Saw);

    const auto t0 = std::chrono::steady_clock::now();
    for (int rep = 0; rep < kIterations; ++rep) {
        for (std::size_t i = 0; i < kBatch; ++i) {
            (void)osc.nextSample();
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double benchBatch() {
    Oscillator osc;
    osc.setFrequency(440.0f, kSampleRate);
    osc.setWaveform(Waveform::Saw);
    std::vector<float> buf(kBatch, 0.0f);

    const auto t0 = std::chrono::steady_clock::now();
    for (int rep = 0; rep < kIterations; ++rep) {
        oscillatorRenderBatch(osc, Waveform::Saw, buf.data(), kBatch);
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace

int main() {
    const double scalarMs = benchScalar();
    const double batchMs = benchBatch();
    const double samples = static_cast<double>(kIterations) * static_cast<double>(kBatch);
    const double scalarMbps = samples / scalarMs / 1000.0;
    const double batchMbps = samples / batchMs / 1000.0;

    std::printf("RACE oscillator benchmark (%d × %zu samples)\n", kIterations, kBatch);
#if defined(RACE_NEON)
    std::printf("NEON: enabled (Apple Silicon / aarch64)\n");
#else
    std::printf("NEON: disabled (scalar batch fallback)\n");
#endif
    std::printf("  scalar nextSample():  %.2f ms  (%.2f M samples/s)\n", scalarMs, scalarMbps);
    std::printf("  batch render:          %.2f ms  (%.2f M samples/s)\n", batchMs, batchMbps);
    std::printf("  speedup:               %.2fx\n", scalarMs / batchMs);
    return 0;
}
