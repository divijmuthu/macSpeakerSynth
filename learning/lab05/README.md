# Lab 05 — Biquad low-pass filter

> Status note: for latest project-wide behavior and defaults, see [CURRENT_STATE.md](../CURRENT_STATE.md).

**Goal:** Sculpt brightness with a second-order IIR low-pass — the core of **subtractive synthesis**.

**Prerequisites:** Lab 04 green (`./build/test_lab04`, live keyboard works).

**Read:** [ROADMAP.md](../ROADMAP.md)

---

## What Lab 05 contributes

```text
  BEFORE (Lab 04):  osc × envelope  →  speakers

  AFTER (Lab 05):   osc × envelope  →  biquad LPF  →  speakers
                                         ↑
                              CUTOFF,1200 from Python
```

| Piece | Role |
|-------|------|
| `Biquad` | IIR filter math — state + coefficients |
| `Voice::nextSample` | Inserts filter after envelope |
| `CUTOFF,<hz>` wire msg | Live cutoff from Python (same protocol as Lab 04) |
| `drainControlQueue` | Already handles `Cutoff` — wired for you |

---

## Files you edit

| File | Questions |
|------|-----------|
| `src/DSP_utilities/Biquad.cpp` | Q1 coefficients, Q2 `process()` |
| `src/DSP_utilities/Voice.cpp` | Q3 route through filter |

---

## Build & run

```bash
cmake --build build
./build/test_lab05

./build/race_synth              # terminal 1
python py_interface/main.py     # terminal 2
```

Play a note (`h`), then sweep cutoff:

```text
> c 500      # dull / muffled
> c 12000    # bright
```

---

## Where do `w0`, `alpha`, and the coefficients come from?

You do **not** need to derive the Cookbook formulas in Lab 05 — but you should know the **story** they fit into.

### 1. What you want (time domain)

A **low-pass** lets slow wiggles through and suppresses fast wiggles:

```text
  input x[n]  ──► [ filter ] ──► output y[n]
                  "remember & smooth"
```

The biquad implements that with **5 multiplies and 4 delays** — the recurrence you already coded in `process()`.

### 2. Same thing in the z-domain (why DSP courses love this)

Take the z-transform of the recurrence (replace delay-by-one with \(z^{-1}\)):

```text
  Y(z) = H(z) · X(z)

  H(z) = (b0 + b1·z⁻¹ + b2·z⁻²) / (1 + a1·z⁻¹ + a2·z⁻²)
         ─────────────────────   ─────────────────────
              numerator               denominator
              (zeros)                 (poles)
```

- **Poles** (roots of denominator) ≈ "how the filter rings / remembers"
- **Zeros** (roots of numerator) ≈ "frequencies we notch or kill"
- **Low-pass:** poles near DC (0 Hz), zeros that kill Nyquist / highs

You do **not** compute `H(z)` each sample in real time. You pick pole/zero locations once → get `(b0…b2, a1, a2)` → run the recurrence forever.

### 3. Filter design pipeline (Cookbook = step 3 done for you)

```text
  (A) Draw analog prototype     s-domain: simple RC-style low-pass
           │
           ▼  bilinear transform  s → (2/fs)·(1-z⁻¹)/(1+z⁻¹)
  (B) Map to digital filter     z-domain biquad at sample rate fs
           │
           ▼  algebra
  (C) Publish b0,b1,b2,a0,a1,a2   Robert Bristow-Johnson "Audio EQ Cookbook"
           │
           ▼  your code
  (D) setLowPass() → normalize by a0 → process() each sample
```

**Bilinear transform** warps analog frequency to digital so the filter is stable on the computer. That warp is exactly why the intermediate variable

```text
  w0 = 2π · fc / fs
```

appears: **`fc` is your cutoff in Hz**, `fs` is sample rate, and `w0` is "where cutoff lives" on the **digital** unit circle (radians per sample).

### 4. What the Cookbook symbols mean (one line each)

| Symbol | Meaning | Intuition |
|--------|---------|-----------|
| **`w0`** | \(2\pi f_c / f_s\) | Cutoff as digital angle; 0 = DC, π = Nyquist |
| **`cos(w0)`** | cosine of that angle | Positions conjugate pole pair on z-plane |
| **`Q`** | quality factor | Higher Q = narrower peak / more resonance near cutoff |
| **`alpha`** | \(\sin(w_0) / (2Q)\) | Derived bandwidth knob in the cookbook algebra |
| **`b0…b2`** | feed-forward taps | Mix current + past **inputs** |
| **`a1, a2`** | feedback taps | Mix past **outputs** (IIR = infinite impulse response) |
| **`a0`** | leading denominator coeff | Normalize so `a0 = 1` in code |

The low-pass **b0,b1,b2** all involve `(1 - cos(w0))` because that shape corresponds to a symmetric low-pass numerator.

### 5. C++ gotcha (your compile error)

Cookbook is **math notation**. C++ needs **declared variables**:

```cpp
// Math on paper          →  Valid C++
// w0 = 2π fc / fs        →  const float w0 = 2.0f * kPi * fc / sampleRate;
// pi                     →  kPi (or M_PI)
// b0_ = b0 / a0          →  b0_, a0 are different: locals b0,a0 then assign to members b0_
```

Undeclared `w0`, `pi`, `b0` → compiler errors. Members are `b0_`, `b1_`, …; locals in `setLowPass` should be `const float b0 = …`.

---

## Concept: biquad as a recurrence

The filter remembers past inputs and outputs:

```text
  y[n] = b0·x[n] + b1·x[n-1] + b2·x[n-2] - a1·y[n-1] - a2·y[n-2]
```

**Intuition:** low-pass = average-ish behavior over time → high frequencies (rapid wiggles) get suppressed.

**LTI:** with fixed coefficients, the same input always produces the same output shape.

---

## Concept check

1. Why run `setLowPass` on the audio thread (via queue drain), not in Python?  
   *(Coefficients touch filter state; must stay in sync with `process()` thread.)*

2. At 48 kHz, cutoff 200 Hz vs 12 kHz — which passes 440 Hz sine better?  
   *(12 kHz — cutoff is above 440.)*

3. Why is a sine wave a forgiving test for a low-pass?  
   *(Single frequency — easy to predict attenuation when cutoff moves below/above it.)*

---

## Question 1 — `setLowPass` coefficients

**File:** `src/DSP_utilities/Biquad.cpp`

See the **"Where do w0, alpha…"** section above, then implement:

```cpp
const float w0 = 2.0f * kPi * fc / sampleRate;
const float cosW0 = std::cos(w0);
const float alpha = std::sin(w0) / (2.0f * q);
// … b0, b1, b2, a0, a1, a2 …
b0_ = b0 / a0;  // members have trailing underscore
```

Clamp `cutoffHz` to `(0, fs/2)` so coefficients stay sane.

---

## Question 2 — `process`

**Already implemented** if your recurrence matches the comment. Verify delay order: compute `y`, **then** shift `x1,x2,y1,y2`.

---

## Question 3 — wire into `Voice`

**Already done** if `Voice.cpp` returns `filter_.process(raw)`.

---

## What the tests check

| Test | Validates |
|------|-----------|
| `dc passes` | Q1+Q2 — steady input ≈ steady output |
| `attenuates highs` | 100 Hz vs 8 kHz through 500 Hz LPF |
| `open vs closed` | 440 Hz through wide vs narrow cutoff |
| `parse CUTOFF` | wire protocol (already in parser) |
| `Voice filter` | Q3 — lower cutoff quiets tone |
| `drain CUTOFF` | end-to-end control path |

---

## When you are done

1. `./build/test_lab05` all `ok`
2. Live: note + `c 300` vs `c 10000` — hear brightness change
3. Check off Lab 05 on [ROADMAP.md](../ROADMAP.md)

Next: [lab06/README.md](../lab06/README.md) — state-variable filter.

---

## Stretch

- Add `Q` control message for resonance (`CUTOFF,800` + `RESONANCE,1.5`)
- Switch oscillator to sawtooth (Lab 06 preview) — filter becomes much more obvious
