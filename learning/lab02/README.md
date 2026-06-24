# Lab 02 — ADSR envelope & Voice

> Status note: for latest project-wide behavior and defaults, see [CURRENT_STATE.md](../CURRENT_STATE.md).

**Goal:** Shape note volume over time and wire it to your Lab 01 oscillator inside a `Voice` — the same object Lab 03 drops into the audio callback.

**Prerequisites:** Lab 01 complete (`./build/test_lab01` all green).

**Read first:** [ROADMAP.md](../ROADMAP.md) — see where `Voice::nextSample()` lands in the finished synth.

---

## Why this lab exists

A raw sine at full volume sounds fine in tests. In speakers, **instantly** jumping from 0 → full scale causes a **click** (discontinuity). Real synths multiply the waveform by an **envelope** — a gain from 0.0 to 1.0 that moves on human time scales (milliseconds), not audio time scales (microseconds).

After this lab, one line of your future callback is:

```cpp
float sample = voice.nextSample();  // osc × envelope — you build this today
```

---

## The ADSR state machine

```text
                    noteOn()
         Idle ─────────────────► Attack
          ▲                         │
          │                         │ level → 1.0
          │                         ▼
          │                       Decay
          │                         │
          │                         │ level → sustainLevel
          │                         ▼
          │                      Sustain ──── (hold while key down)
          │                         │
          │              noteOff()  │
          │                         ▼
          └──────────────────── Release
                    level → 0.0
```

| Stage | What happens | Typical time |
|-------|----------------|--------------|
| **Attack** | 0 → 1.0 | 5–50 ms |
| **Decay** | 1.0 → sustain | 50–300 ms |
| **Sustain** | Hold at ~0.7 | While key held |
| **Release** | sustain → 0 | 100–500 ms |

We use **linear ramps** (add/subtract a fixed `delta` per sample). Pros: easy to reason about. Cons: not as natural as exponential curves — fine for learning.

---

## Files you edit

| File | Questions |
|------|-----------|
| `src/DSP_utilities/Envelope.cpp` | Q1, Q2a, Q2b, Q3 |
| `src/DSP_utilities/Voice.cpp` | Q4 |

Headers are provided — read them for the API.

**Run tests:**

```bash
cmake --build build && ./build/test_lab02
```

---

## Concept check

1. If `attackSec = 0.01` and `sampleRate = 48000`, how many samples does Attack last?  
   *(Answer: 480 samples)*

2. What should `level_` be at the exact moment you call `noteOn()`?  
   *(Answer: 0.0 — retrigger from silence)*

3. Why must Release **not** snap `level_` to zero immediately?  
   *(Answer: same discontinuity → click)*

4. In `Voice::nextSample()`, why multiply **after** calling `env_.process()`?  
   *(Answer: gain applies to this sample's time step)*

---

## Question 1 — `configure`: attack & decay deltas

**File:** `src/DSP_utilities/Envelope.cpp`  
**TODO:** `Lab 02, Q1`

Precompute per-sample steps for Attack and Decay:

```cpp
const float attackSamples = attackSec_ * sampleRate_;
attackDelta_ = 1.0f / attackSamples;

const float decaySamples = decaySec_ * sampleRate_;
decayDelta_ = (1.0f - sustainLevel_) / decaySamples;
```

Guard against division by zero in your head: tests use sane times (≥ 0.01 s).

**Do not** put `releaseDelta_` here — release starts from whatever level you have when the key lifts (see Q2b).

---

## Question 2a — `noteOn`

**TODO:** `Lab 02, Q2a`

- `stage_ = EnvelopeStage::Attack`
- `level_ = 0.0f`

---

## Question 2b — `noteOff`

**TODO:** `Lab 02, Q2b`

- If `stage_ == Idle`, return immediately.
- Set `stage_ = EnvelopeStage::Release`.
- Compute release rate from **current** level (so releasing during Decay still works):

```cpp
const float releaseSamples = releaseSec_ * sampleRate_;
releaseDelta_ = level_ / releaseSamples;
```

---

## Question 3 — `process`: the FSM loop body

**TODO:** `Lab 02, Q3`

One call = one audio sample. Use a `switch (stage_)` (or if-chain):

| Stage | Update | Transition when |
|-------|--------|-----------------|
| Attack | `level_ += attackDelta_` | `level_ >= 1.0f` → clamp to 1.0, go Decay |
| Decay | `level_ -= decayDelta_` | `level_ <= sustainLevel_` → clamp, go Sustain |
| Sustain | no change | — |
| Release | `level_ -= releaseDelta_` | `level_ <= 0.0f` → clamp to 0, go Idle |
| Idle | `level_ = 0.0f` | — |

Always `return level_`.

**Edge case:** use `std::min` / `std::max` or manual clamps so level never overshoots.

---

## Question 4 — `Voice::nextSample`

**File:** `src/DSP_utilities/Voice.cpp`  
**TODO:** `Lab 02, Q4`

```cpp
const float gain = env_.process();
const float raw = osc_.nextSample();
return raw * gain;
```

This three-line kernel is **the inner loop of your synthesizer**. Lab 03 wraps it in a `for` over the output buffer.

---

## Integration preview (Lab 03 — do not implement yet)

```cpp
// AudioCore callback — coming in Lab 03
void data_callback(ma_device* device, void* output, ...) {
    float* out = static_cast<float*>(output);
    auto* voice = static_cast<Voice*>(device->pUserData);

    for (std::uint32_t i = 0; i < frameCount; ++i) {
        const float s = voice->nextSample();
        out[2 * i + 0] = s;
        out[2 * i + 1] = s;
    }
}
```

---

## What the tests check

| Test | Validates |
|------|-----------|
| `attack_delta` | Q1 — attackDelta matches formula |
| `decay_delta` | Q1 — decayDelta matches formula |
| `note_on_starts_attack` | Q2a — stage and level after noteOn |
| `attack_reaches_unity` | Q3 — ~1.0 after attackSec |
| `decay_reaches_sustain` | Q3 — ~sustainLevel after attack+decay |
| `release_returns_to_idle` | Q2b+Q3 — noteOff → level 0, stage Idle |
| `voice_silent_before_note_on` | Q4 — Voice output ~0 when envelope idle |
| `voice_audible_during_sustain` | Q4 — osc×env peak during held note |
| `no_click_retrigger` | Q2a — second noteOn resets cleanly |

---

## When you are done

All `test_lab02` checks print `ok`. Then:

1. Mark Lab 02 complete on [ROADMAP.md](../ROADMAP.md).
2. Open [lab03/README.md](../lab03/README.md) — speakers and miniaudio.

---

## Stretch goals (optional)

- Exponential attack (sounds more natural): `level_ += (1.0f - level_) * attackCoef`
- Add `isActive()` early-out in `Voice::nextSample()` to skip `sin()` when silent (CPU win at scale)
