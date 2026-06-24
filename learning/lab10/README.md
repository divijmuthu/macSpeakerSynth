# Lab 10 — Assignment: SIMD / NEON Performance Investigation

> Status note: see [CURRENT_STATE.md](../CURRENT_STATE.md) for current behavior.
> Measured data: [PERFORMANCE_TRACKING.md](../PERFORMANCE_TRACKING.md).

**Goal:** Understand and verify the vectorized oscillator path, then explain its performance tradeoffs in interview-quality language.

---

## Assignment outcomes

By the end you should be able to answer:

1. What does SIMD change in the oscillator workload?
2. How is NEON enabled/disabled at build time?
3. Why can an initial SIMD implementation still benchmark near 1x?

---

## Part A — Read the implementation

Focus files:

- `include/DSP_utilities/SimdOscillator.h`
- `src/DSP_utilities/SimdOscillator.cpp`
- `tests-and-benchmarks/benchmark_neon.cpp`
- `CMakeLists.txt` (`RACE_ENABLE_NEON`, `RACE_NEON`)

Task:

- Trace how scalar and batch paths are selected.
- Write a short note (5-10 lines) describing where NEON is used and where scalar math remains.

---

## Part B — Run controlled benchmarks

### Build with NEON on

```bash
cmake -S . -B build-neon -DRACE_ENABLE_NEON=ON -DRACE_USE_COREAUDIO=ON
cmake --build build-neon --target benchmark_neon
./build-neon/benchmark_neon
```

### Build with NEON off

```bash
cmake -S . -B build-scalar -DRACE_ENABLE_NEON=OFF -DRACE_USE_COREAUDIO=ON
cmake --build build-scalar --target benchmark_neon
./build-scalar/benchmark_neon
```

Task:

- Record both outputs in `learning/PERFORMANCE_TRACKING.md`.
- Add one paragraph interpreting results.

---

## Part C — Explain tradeoffs (interview style)

Prepare a response covering:

- Why SIMD matters for polyphony.
- Why this benchmark may not show >1x speedup yet.
- What changes would likely improve speedup (e.g., deeper integration and vector-friendly waveform math).

---

## Canonical reference implementation

Lab 10 baseline is already implemented in this repo.
Use it as the reference while writing your own explanation and measurements.

---

## Completion checklist

- [ ] I can point to where `RACE_NEON` is defined.
- [ ] I can run ON/OFF benchmarks.
- [ ] I can explain the current speedup result and next optimization steps.

Next: [lab11/README.md](../lab11/README.md)
