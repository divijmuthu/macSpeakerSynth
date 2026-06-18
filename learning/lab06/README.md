# Lab 06 — State-variable filter (TPT / SVF)

**Goal:** Replace the biquad in `Voice` with a **trapezoidal state-variable filter** — smoother cutoff sweeps and **three modes** (LP / HP / BP) from one state.

**Prerequisites:** Lab 05 green (`./build/test_lab05`).

**Read:** [ROADMAP.md](../ROADMAP.md) · Lab 05 biquad section for contrast.

---

## What Lab 06 contributes

```text
  Lab 05:  osc × env → Biquad (one mode, direct form, coeff recalc)

  Lab 06:  osc × env → SVF (LP / HP / BP, TPT — stable when modulated)
                           ↑
                    MODE,0|1|2 from Python
```


| vs Biquad (Lab 05)                       | SVF (Lab 06)                                          |
| ---------------------------------------- | ----------------------------------------------------- |
| Recompute `b0…a2` on every cutoff change | Update `**g = tan(π·fc/fs)**` only                    |
| One output (low-pass)                    | **LP, HP, BP** from same `v1`, `v2`                   |
| Direct Form I                            | **TPT / ZDF** — better when cutoff moves every buffer |


`Biquad` stays in the repo (Lab 05 tests still use it). `**Voice` now uses `StateVariableFilter`.**

---

## Files you edit


| File                                        | Questions                                   |
| ------------------------------------------- | ------------------------------------------- |
| `src/DSP_utilities/StateVariableFilter.cpp` | Q1 `setCutoff`, Q2 TPT tick, Q3 mode output |


`Voice`, parser, `AudioCore` drain for `MODE` — **already wired**.

---

## Build & run

```bash
cmake --build build
./build/test_lab06

./build/race_synth
lpython py_interface/main.py
```

```text
> h          # note
> c 800      # cutoff
> m 0        # low-pass
> m 1        # high-pass  (thin, bright)
> m 2        # band-pass   (nasally / telephone-ish)
```

---

## Intuition: one filter, three outputs

Classic **analog** state-variable topology integrates input into **band-pass** and **low-pass** states; **high-pass** is what's left:

```text
         ┌──► HP = input - k·BP - LP
         │
  in ──► │ integrator chain ──► LP (smooth = low frequencies)
         │
         └──► BP (in-between)
```

Digital **TPT** version (what you implement) keeps two **trapezoidal state** variables `ic1eq`, `ic2eq` instead of baking huge coefficient tables.

### Why `g = tan(π·fc/fs)`?

Same story as Lab 05's `w0`, but TPT prewarp uses **tan** so the **cutoff frequency lands correctly** after the bilinear-style warping. You only update `**g` and `k`** when cutoff changes — no full biquad fraction recalc.


| Variable                | Role                                                      |
| ----------------------- | --------------------------------------------------------- |
| `**g**`                 | `tan(π·fc/fs)` — cutoff warp                              |
| `**k**`                 | `1/Q` — resonance (peak near cutoff)                      |
| `**ic1eq`, `ic2eq**`    | Filter memory (like biquad delay lines, different layout) |
| `**v1`, `v2**`          | Intermediate states this sample                           |
| `**lp_`, `bp_`, `hp_**` | Three outputs from **one** tick                           |


---

## Question 1 — `setCutoff`

**TODO:** `Lab 06, Q1`

```cpp
const float fc = std::clamp(cutoffHz, 20.0f, sampleRate * 0.49f);
g_ = std::tan(kPi * fc / sampleRate);
k_ = 1.0f / q;
```

---

## Question 2 — TPT tick

**TODO:** `Lab 06, Q2`

```cpp
const float v1 = (ic1eq_ + g_ * (input - ic2eq_)) / (1.0f + g_ * (g_ + k_));
const float v2 = ic2eq_ + g_ * v1;

lp_ = v2;
bp_ = v1;
hp_ = input - k_ * v1 - v2;

ic1eq_ = 2.0f * v1 - ic1eq_;
ic2eq_ = 2.0f * v2 - ic2eq_;
```

---

## Question 3 — pick mode

**TODO:** `Lab 06, Q3`

```cpp
switch (mode_) {
    case FilterMode::LowPass:  return lp_;
    case FilterMode::HighPass: return hp_;
    case FilterMode::BandPass: return bp_;
}
```

---

## Wire protocol (already added)

```text
MODE,0   → LowPass
MODE,1   → HighPass
MODE,2   → BandPass
```

---

## What the tests check


| Test                  | Validates                                 |
| --------------------- | ----------------------------------------- |
| `SVF LP`              | Q1–Q3 low-pass attenuates 8 kHz vs 100 Hz |
| `SVF HP`              | High-pass kills 80 Hz vs 4 kHz            |
| `SVF BP`              | Band-pass peaks near fc                   |
| `multimode same tick` | LP/BP/HP differ on one `process()`        |
| `parse MODE`          | wire protocol                             |
| `Voice HP`            | end-to-end                                |
| `drain MODE`          | queue → voice mode                        |


---

## Note on Lab 05 regression

`test_lab05` **Voice** tests expect a working **low-pass**. After Lab 06 Q1–Q3, SVF LP should satisfy them too. Direct **Biquad** tests in `test_lab05` still test Lab 05 in isolation.

---

## When you are done

1. `./build/test_lab06` all `ok`
2. Play with `m 0/1/2` while holding a note
3. Check off Lab 06 on [ROADMAP.md](../ROADMAP.md)

Next: [lab07/README.md](../lab07/README.md) — delay + saturation.

---

## Stretch

- Coefficient **smoothing**: lerp `g_` toward target each sample (zipper-free cutoff sweeps)
- Add **resonance** message `RESONANCE,1.8` mapping to `Q`

