# Lab 01 — Phase accumulator & sine wave

> Status note: for latest project-wide behavior and defaults, see [CURRENT_STATE.md](../CURRENT_STATE.md).

**Goal:** Implement a tiny `Oscillator` class that generates one sine sample at a time using a **phase accumulator**. No speakers yet — only unit tests.

**Prerequisites:** Basic C++ (functions, structs/classes). No audio API knowledge required.

**Files you edit:**

- `include/DSP_utilities/Oscillator.h` — read only; interface is given
- `src/DSP_utilities/Oscillator.cpp` — **your TODOs live here**

**Run tests:**

```bash
cmake -S . -B build && cmake --build build && ./build/test_lab01
```

---

## Concept: why not just `sin(2π × f × t)` every sample?

You *could* compute absolute time `t` for each sample and call `sin(2πft)`. That works in a homework script. In a real synth running 48,000 times per second, you want something cheaper and numerically stable.

Instead, keep a **phase** variable (angle in radians) and **add a fixed step** each sample:

```text
  sample[n] = sin(phase)
  phase     = phase + phaseIncrement
  if phase >= 2π → wrap back into [0, 2π)
```

The step depends on frequency and sample rate:

```text
  phaseIncrement = 2π × frequency / sampleRate
```

**Intuition:** If frequency = sampleRate, you want exactly one full cycle (2π) per sample → increment = 2π. If frequency is half sampleRate, one cycle every two samples → increment = π.

---

## Concept check (before coding)

Answer these in your head or on paper:

1. At **440 Hz** and **48000 Hz** sample rate, roughly how many samples make up one full cycle?  
   *(Hint: sampleRate / frequency ≈ ?)*

2. After 48000 consecutive samples at 440 Hz, how many complete cycles should you have produced?  
   *(Hint: frequency × 1 second = ?)*

3. Why do we **wrap** phase instead of letting it grow forever?

If (1) ≈ 109 and (2) = 440, you are in good shape.

---

## Question 1 — `setFrequency`

**File:** `src/DSP_utilities/Oscillator.cpp`  
**TODO marker:** `TODO (Lab 01, Q1)`

Store the correct **phase increment** in `phaseIncrement_` when the user (or test) sets frequency and sample rate.

**Formula:**

```cpp
phaseIncrement_ = 2.0f * M_PI * freqHz / sampleRate;
```

Use `M_PI` from `<cmath>` (already included via the header chain).

**Hint:** Units check — `freqHz / sampleRate` is “cycles per sample”; multiply by `2π` to get “radians per sample.”

---

## Question 2 — `nextSample` (the sine value)

**TODO marker:** `TODO (Lab 01, Q2)`

Return `sin(phase_)` **before** you advance the phase (order matters for tests).

---

## Question 3 — advance and wrap phase

**TODO marker:** `TODO (Lab 01, Q3)`

After computing the sample:

1. Add `phaseIncrement_` to `phase_`.
2. Wrap: while `phase_ >= 2π`, subtract `2π`.  
   *(A `while` loop is fine for Lab 01; there are faster tricks later.)*

Do **not** reset phase to zero every sample — that would click.

---

## What the tests check

| Test | Idea |
|------|------|
| `test_phase_increment` | Q1 — increment matches formula for 440 Hz @ 48 kHz |
| `test_first_sample_is_zero` | Phase starts at 0 → first `sin(0) == 0` |
| `test_peak_near_quarter_cycle` | After ~¼ cycle of samples, value near +1 |
| `test_one_second_of_cycles` | 48000 samples at 440 Hz → phase wraps ~440 times (via amplitude pattern) |

---

## When you are done

All four tests print `ok`. Then:

1. See how `Oscillator` fits the full synth → [ROADMAP.md](../ROADMAP.md)
2. Open **[lab02/README.md](../lab02/README.md)** — ADSR envelope + `Voice` (osc × env)

---

## Where Lab 01 lands in the final callback

You built the **Oscillator** block. In the finished synth, every sample looks like:

```text
  raw = oscillator.nextSample()     ← Lab 01 (you)
  gain = envelope.process()         ← Lab 02
  out = filter.process(raw * gain)  ← Lab 05
```

Lab 03 wraps that in a loop over the speaker buffer. Nothing you wrote here gets thrown away.

---

## Optional stretch (not graded by tests)

- Add a `Waveform` enum (Sine only for now) so Lab 06 can branch on shape.
- Plot 100 samples to a CSV and view in a spreadsheet — see the sine visually.
