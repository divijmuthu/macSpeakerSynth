# Performance Tracking (Lab 10/11)

This file records concrete measurements and how to reproduce them.

## Benchmark target

`tests-and-benchmarks/benchmark_neon.cpp`

Measures oscillator throughput on:

- scalar `nextSample()`
- batch `oscillatorRenderBatch()`

for `2000 x 256` samples.

## Repro commands

NEON enabled:

```bash
cmake -S . -B build-neon -DRACE_ENABLE_NEON=ON -DRACE_USE_COREAUDIO=ON
cmake --build build-neon --target benchmark_neon
./build-neon/benchmark_neon
```

NEON disabled (scalar fallback build):

```bash
cmake -S . -B build-scalar -DRACE_ENABLE_NEON=OFF -DRACE_USE_COREAUDIO=ON
cmake --build build-scalar --target benchmark_neon
./build-scalar/benchmark_neon
```

## Latest recorded run (Apple Silicon, Jun 2026)

### NEON ON

- scalar nextSample: `2.45 ms` (`208.61 M samples/s`)
- batch render: `2.68 ms` (`191.08 M samples/s`)
- reported speedup: `0.92x`

### NEON OFF

- scalar nextSample: `3.76 ms` (`136.26 M samples/s`)
- batch render: `3.79 ms` (`135.25 M samples/s`)
- reported speedup: `0.99x`

## Interpretation

- The build with NEON enabled improves overall throughput vs NEON-off build.
- Current batch path is not yet faster than scalar in this specific benchmark because:
  - sine path still uses scalar `std::sin` per lane in helper code,
  - batching overhead can dominate at this workload size,
  - path is currently a demonstrator and not deeply integrated in full voice rendering.

This is normal for an educational first SIMD pass. The major value is architecture readiness and controlled measurement.

## Core Audio usage check (Lab 11)

Build-time status in CMake:

- `RACE: macOS Core Audio backend enabled` (when `RACE_USE_COREAUDIO=ON` on Apple)

Runtime status (from `AudioCore::start()`):

- prints backend name, e.g. `Audio backend: Core Audio (macOS HAL)`

## Next optimization ideas

1. Replace lane-wise transcendental calls with vector-friendly approximations where acceptable.
2. Move batch rendering into polyphonic voice loop for realistic end-to-end gains.
3. Add callback-time instrumentation (XRuns/jitter counters) for backend comparison.
