# Making Polyphony Sound Better — Research Notes

This is a prioritized, implementable list of why stacked notes sound harsh/buzzy
today and what fixes give the biggest perceptual win per unit of effort.

## TL;DR ranking (do these in order)

1. **Band-limit the oscillators (PolyBLEP).** ~80% of the "buzzy chord" problem.
2. **Sum with a soft bus limiter instead of `1/N` + hard clamp.** Fixes "chords go quiet then suddenly pop/clip."
3. **Add slight per-voice detune / drift.** Turns a sterile stack into a "fat" chord.
4. **Fade stolen voices (1–5 ms) instead of hard re-init.** Removes voice-steal clicks.
5. **Remove DC + add a gentle master high-pass.** Cleans up asymmetric-wave stacks.

---

## 1. Aliasing is the #1 culprit (band-limited oscillators)

Today `Waveform.cpp` generates **naive** saw and square:

```cpp
case Waveform::Saw:    return 2.0f * (phase / kTwoPi) - 1.0f;   // hard ramp
case Waveform::Square: return phase < kPi ? 1.0f : -1.0f;       // hard step
```

The instantaneous jump at each cycle boundary contains energy at *every* harmonic,
including harmonics above Nyquist (24 kHz @ 48 kHz). Those fold back down as
**inharmonic** frequencies. With one note it's tolerable; with a **chord** every
voice contributes its own fold-back partials and they beat against each other →
the "sharp/harsh/buzzy" sound you hear.

**Fix: PolyBLEP** (Polynomial Band-Limited Step). At each discontinuity, subtract a
small correction polynomial spanning ±1 sample. Cheap (a few ops), real-time safe,
no tables.

```cpp
// t = phase normalized to [0,1), dt = phaseIncrement / 2π (cycles per sample)
static float polyBlep(float t, float dt) {
    if (t < dt)            { t /= dt;        return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)     { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}
// band-limited saw:  naiveSaw - polyBlep(t, dt)
// band-limited square: naiveSquare + polyBlep(t,dt) - polyBlep((t+0.5)mod1, dt)
// band-limited triangle: leaky-integrate the band-limited square
```

This is the single highest-impact change for chord quality. Wavetables or BLIT are
alternatives but PolyBLEP is the best effort/quality tradeoff here.

## 2. Bus summing & headroom (replace `0.9/N` + hard clamp)

Current `mixActiveVoices()`:

```cpp
targetGain = (activeCount == 1) ? 1.0f : (0.9f / activeCount);
...
return std::clamp(mix, -0.98f, 0.98f);   // hard knee
```

Problems:
- `1/N` is **too aggressive** — a 4-note chord drops to ~0.22 gain, so chords feel
  weak and you crank master gain, which then clips on transients.
- The `std::clamp` is a **hard** limiter — any overshoot becomes a square edge =
  more harsh harmonics (same aliasing problem, now on the master bus).

Better options:
- Scale by **`1/sqrt(N)`** (equal-power) instead of `1/N`. Uncorrelated voices sum
  in power, not amplitude, so `1/sqrt(N)` keeps perceived loudness roughly constant
  across chord sizes.
- Replace the hard clamp with a **soft saturator / one-pole limiter** (you already
  have `SoftClip` — route the master bus through a gentle `tanh` always-on stage, or
  a lookahead-free peak limiter with ~5 ms release).

## 3. Per-voice detune / drift (richness)

Real analog polysynths are never perfectly in tune across voices. Add a tiny random
or fixed cents offset per voice (e.g. ±3–7 cents) and/or a slow random LFO on pitch.
Beating between near-unison partials is what makes a chord sound "fat" instead of
"digital." Cheap: multiply each voice frequency by `pow(2, cents/1200)` at note-on.

## 4. Voice stealing without clicks

`allocateVoice()` steals the oldest slot and immediately re-inits it. If that slot
was mid-note, the abrupt amplitude jump is a click. Give the stolen voice a very
short (1–5 ms) release ramp before reassigning, or cross-fade. We already randomize
phase and reset the filter on a *fresh* note; the missing piece is the **fade-out of
the victim**.

## 5. DC offset + master high-pass

Saw and (asymmetric) pulse waves carry DC. Summing several voices accumulates DC
offset, eating headroom asymmetrically and making the soft clipper distort earlier
on one side. A one-pole DC blocker / 20–30 Hz high-pass on the master bus is ~3 lines
and removes that.

---

## What we already do well (keep these)

- **Per-voice filter + ADSR** before summing (correct order: `filter(osc) * gain`).
- **Random start phase per voice** → reduces phase-locked comb filtering on chords.
- **Gain slew** (`mixGainCurrent_`) → no zipper noise when voice count changes.
- **Effects default to bypass** → users hear pure tones first.

## Suggested next implementation order

| Step | Change | File(s) | Effort | Payoff |
|------|--------|---------|--------|--------|
| 1 | PolyBLEP saw/square/tri | `Waveform.cpp`, `Oscillator` (needs `dt`) | Medium | **Huge** |
| 2 | `1/sqrt(N)` + soft master limiter | `AudioCore.cpp` (`mixActiveVoices`/`processMasterEffects`) | Low | High |
| 3 | Per-voice detune | `Voice`/`AudioCore::allocateVoice` | Low | Medium |
| 4 | Victim fade on steal | `AudioCore::allocateVoice` | Medium | Medium |
| 5 | DC blocker / master HPF | `AudioCore::processMasterEffects` | Low | Low–Med |

Compare A/B against the references in `REFERENCE_IMPLEMENTATIONS.md` after step 1 —
the difference on a 3-note saw chord should be obvious.
