# Lab 05 — Biquad low-pass filter

**Unlock after:** Lab 04.

**Goal:** Implement a second-order IIR low-pass and wire it into `Voice` after the envelope.

**Signal chain:** `filter.process(envelope × oscillator)`

---

## What this lab contributes

| DSP | C++ |
|-----|-----|
| Difference equation \(y[n] = (b_0 x[n] + \ldots - a_1 y[n-1] - a_2 y[n-2]) / a_0\) | Delay state in struct (`x1_, x2_, y1_, y2_`) |
| Cutoff → coefficient mapping (bilinear transform intro) | `setCutoff()` called from control messages (Lab 04) |
| Subtractive synthesis: dull the harmonics | Extend `Voice::nextSample()` — no rewrite of osc/env |

**Files:** `include/DSP_utilities/Biquad.h`, `src/DSP_utilities/Biquad.cpp`, update `Voice.*`

See [ROADMAP.md](../ROADMAP.md).
