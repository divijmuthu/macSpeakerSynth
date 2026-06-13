# Lab 07 — Delay line & soft saturation

**Unlock after:** Lab 06.

**Goal:** Add time-based and nonlinear DSP — a **delay effect** and **soft clipper** on the master bus or per voice.

---

## What this lab contributes

Two classic effect families:

```text
  dry ──────────────────────────────┬──► + ──► out
                                    │
  dry → [DelayLine] → feedback ─────┘
              ↑
         wet/dry mix

  sample → tanh(k·x)/tanh(k)  or  x / (1 + |x|)   ← soft saturation
```

| DSP | C++ |
|-----|-----|
| Circular buffer, read/write pointers | Fixed-size `std::array`, modulo indexing |
| Feedback stability (< 1.0 gain) | Wet/dry as float params from UI |
| Nonlinear waveshaping | Branchless-friendly math, clamp headroom |

**Files:** `DelayLine.h/.cpp`, `SoftClip.h/.cpp`, effect mix in `AudioCore` or `Voice`

See [ROADMAP.md](../ROADMAP.md).
