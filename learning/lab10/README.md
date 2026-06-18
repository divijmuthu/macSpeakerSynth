# Lab 10 — ARM NEON / SIMD batch oscillator

**Goal:** Process **4 samples at a time** on Apple Silicon using `arm_neon.h` — same math, wider lanes.

---

## Why SIMD here?

The audio callback must produce **48,000 samples/sec**. One `nextSample()` call is cheap, but polyphony (Lab 08) multiplies work. SIMD is how real synths keep CPU headroom.

```text
  scalar:  phase → sample → phase → sample …
  NEON:    [p0 p1 p2 p3] → [s0 s1 s2 s3] in one batch
```

---

## Files

| File | Role |
|------|------|
| `include/DSP_utilities/SimdOscillator.h` | `oscillatorRenderBatch()` API |
| `src/DSP_utilities/SimdOscillator.cpp` | NEON path when `RACE_NEON=1` |

CMake enables `RACE_NEON` automatically on `arm64`.

---

## Benchmark

```bash
cmake --build build
./build/benchmark_neon
```

Compare **scalar** vs **batch** throughput (M samples/s).

---

## Integration note

Today batch is tested standalone; `Voice::nextSample()` stays scalar for simplicity. Lab 08 polyphony is the natural place to call batch per voice.

Next: [lab11/README.md](../lab11/README.md) — macOS Core Audio HAL.
