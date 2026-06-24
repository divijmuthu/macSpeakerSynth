# Lab 09 — Waveforms (saw, square, triangle)

> Status note: for latest project-wide behavior and defaults, see [CURRENT_STATE.md](../CURRENT_STATE.md).
> Reference comparisons: [REFERENCE_IMPLEMENTATIONS.md](../REFERENCE_IMPLEMENTATIONS.md).

**Goal:** Hear how **waveform shape** changes timbre before the filter sculpts it — same phase accumulator, different math.

**Prerequisites:** Lab 08 green.

---

## Already implemented

Backend + UI are wired. Your job is to **listen and compare**, then run tests:

```bash
./build/test_lab09
```

Code lives in `Waveform.cpp` → `waveformSample()` and the **Oscillator** panel in the GUI.

---

## Mental model

Same `phase` in `[0, 2π)`, different **shape functions**:

| Waveform | Sound | Harmonics |
|----------|-------|----------|
| **Sine** | Pure, soft | Fundamental only |
| **Saw** | Bright, buzzy | All harmonics (1/n) |
| **Square** | Hollow, clarinet-ish | Odd harmonics only |
| **Triangle** | Mellow, flute-ish | Odd harmonics, faster rolloff |

```text
  phase ──► waveformSample(phase, wf) ──► × envelope ──► filter ──► out
```

---

## What to try (Lab 09 listening session)

1. Start synth + UI. Click the window so **keyboard focus** works.
2. Hold **A + D + F** (C major). Switch waveforms:
   - **Sine** — gentle, pure
   - **Saw** — much brighter; lower cutoff to hear filter sweep matter
   - **Square** — hollow; good with LP around 800 Hz
   - **Triangle** — softer than square
3. Watch the **oscilloscope** — saw/square show sharper edges (aliasing at high pitch).

Wire: `WAVEFORM,0|1|2|3` or GUI radio buttons.

---

## Code reference (`Waveform.cpp`)

```cpp
case Waveform::Saw:
    return 2.0f * (phase / kTwoPi) - 1.0f;
case Waveform::Square:
    return phase < kPi ? 1.0f : -1.0f;
case Waveform::Triangle:
    // piecewise linear ramp up then down
```

---

## Aliasing (why high notes buzz on saw/square)

Naive discontinuous waves **fold** high frequencies back into audible range — digital stair-steps. Lab 09 keeps math simple on purpose.

**Stretch:** BLEP / polyBLEP, wavetables, oversampling.

---

## Tests

```bash
cmake --build build && ./build/test_lab09
```

---

## When you are done

1. `./build/test_lab09` all `ok`
2. You can hear clear timbre differences between waveforms on a chord
3. Optional extensions: [lab10](../lab10/README.md) NEON · [lab11](../lab11/README.md) Core Audio
