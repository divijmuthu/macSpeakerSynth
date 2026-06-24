# Lab 07 — Delay line & soft saturation

> Status note: for latest project-wide behavior and defaults, see [CURRENT_STATE.md](../CURRENT_STATE.md).

**Goal:** Add **echo** (circular delay buffer) and **warm limiting** (soft clip) on the **master bus** after the voice.

**Prerequisites:** Lab 06 green.

---

## The echo in plain English (read this first)

Think of **delay + feedback** like shouting into a canyon:

1. Your voice hits the wall (**dry signal** — you hear it immediately).
2. A copy bounces back **later** (**delay time** — 0.25 s = quarter-second later).
3. That echo hits the wall again and comes back **quieter each time** (**feedback** < 1).
4. **Wet mix** = how loud those echoes are vs your direct voice.

```text
  You play a short note ──► speakers (dry)
                    │
                    └──► [memory buffer] ──wait 250ms──► echo ──► speakers (wet)
                              ▲                              │
                              └──────── feedback 45% ────────┘
```

Nothing magical: it's a **ring buffer** — a fixed array pretending to be "sound memory." Each sample you write into it is read again `delaySamples` later. Feedback feeds part of the delayed signal back in, so one clap becomes *clap … clap … clap …* fading out.

**Why master bus?** The whole mix (one note today, chords in Lab 08) shares one space — like reverb in a room, not an effect per key.

**Soft clip** sits after the mix so loud echoes don't harsh-digital-clip; `tanh` rounds peaks off smoothly.

Try in the UI: **Delay 0.25**, **Feedback 0.45**, **Wet 0.5** — play staccato notes. You should hear distinct repeats.

---

## What Lab 07 contributes

```text
  Voice (osc × env × SVF)
           │
           ▼ dry
  ┌────────────────────────────┐
  │  processMasterEffects()    │  ← Lab 07
  │   delay (wet/dry)          │
  │   softClip (drive)         │
  └────────────────────────────┘
           │
           ▼ speakers
```

| Component | DSP idea | C++ idea |
|-----------|----------|----------|
| `DelayLine` | Echo / comb filter | Fixed `std::array`, modulo index |
| `SoftClip` | Nonlinear saturation | `tanh` or rational curve |
| `AudioCore` | Send/return mix | Wet/dry blend then clip |

---

## Files you edit

| File | Questions |
|------|-----------|
| `src/DSP_utilities/DelayLine.cpp` | Q1 circular buffer + feedback |
| `src/DSP_utilities/SoftClip.cpp` | Q2 waveshaper |
| `src/AudioCore.cpp` | Q3 `processMasterEffects` wet/dry + clip |

---

## Build & run

```bash
cmake --build build
./build/test_lab07
./build/race_synth
python py_interface/main.py
```

---

## Question 1 — `DelayLine::process`

Each sample:

```cpp
readIndex  = (writeIndex + kMax - delaySamples) % kMax
delayed    = buffer[readIndex]
buffer[writeIndex] = input + delayed * feedback
writeIndex = (writeIndex + 1) % kMax
return delayed   // wet tap
```

**Feedback must stay < 1** (we clamp to 0.95) or energy never dies → explosion / crackling.

---

## Question 2 — `SoftClip::process`

```cpp
return std::tanh(drive * input) / std::tanh(drive);
```

---

## Question 3 — `processMasterEffects`

```cpp
const float wet = delayLine_.process(dry);
const float mixed = dry * (1.0f - delayWet_) + wet * delayWet_;
return softClip_.process(mixed);
```

---

## When you are done

1. `./build/test_lab07` all `ok`
2. Hear echo on staccato notes with wet > 0
3. Continue to [lab08/README.md](../lab08/README.md) — polyphony

**Extensions after Lab 08:** [lab09](../lab09/README.md) waveforms · [lab10](../lab10/README.md) NEON · [lab11](../lab11/README.md) Core Audio
