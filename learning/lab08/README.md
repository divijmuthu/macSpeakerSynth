# Lab 08 — Polyphony & voice mixing

> Status note: this lab has active tuning iterations; check [CURRENT_STATE.md](../CURRENT_STATE.md) first.
> Reference comparisons: [REFERENCE_IMPLEMENTATIONS.md](../REFERENCE_IMPLEMENTATIONS.md).

**Goal:** Play **chords** — up to **8 simultaneous notes**, each with its own `Voice`.

**Prerequisites:** Lab 07 green.

---

## What changes

```text
  Before (Lab 07):  one Voice  →  effects  →  speakers
  After (Lab 08):   Voice[0..7] summed →  effects  →  speakers
```

| Idea | Implementation |
|------|----------------|
| Voice pool | `std::array<VoiceSlot, 8>` in `AudioCore` |
| NOTE_ON | Pick a free slot (or steal oldest) |
| NOTE_OFF | Release slot matching that frequency |
| Mix | Sum active voices, clamp to [-1, 1] |

Same threading model — **all voice work stays in the audio callback**.

---

## Wire protocol update

```text
NOTE_ON,440.0        start / retrigger that pitch
NOTE_OFF,440.0       release the voice playing 440 Hz
NOTE_OFF,0           release every active voice
```

The Python UI now **holds multiple keys** and sends frequency on NOTE_OFF.

---

## Files you edit

| File | Questions |
|------|-----------|
| `src/AudioCore.cpp` | Q1 `allocateVoice` · Q2 `releaseVoice` · Q3 `mixActiveVoices` |

`Voice::isActive()` and the voice pool are **already wired**.

---

## Build & run

```bash
cmake --build build
./build/test_lab08

./build/race_synth
python py_interface/main.py
```

Hold **C + E + G** together — C major chord.

---

## Question 1 — `allocateVoice`

On NOTE_ON:

1. **Retrigger:** if a slot already plays this frequency (~0.5 Hz tolerance), `noteOn()` that slot again.
2. **Free slot:** first slot where `!voice.isActive()`.
3. **Steal:** if all 8 busy, take the slot with the **smallest** `allocGeneration` (oldest note).

```cpp
slot.frequencyHz = frequencyHz;
slot.allocGeneration = ++allocCounter_;
slot.voice.setFrequency(frequencyHz, kSampleRate);
slot.voice.noteOn();
return slotIndex;
```

---

## Question 2 — `releaseVoice`

```cpp
if (frequencyHz == 0.0f) {
    // noteOff every active slot
    return;
}
// find active slot where slot.frequencyHz ≈ frequencyHz → noteOff()
```

---

## Question 3 — `mixActiveVoices`

```cpp
float mix = 0.0f;
for (auto& slot : voices_) {
    if (slot.voice.isActive()) {
        mix += slot.voice.nextSample();
    }
}
return std::clamp(mix, -1.0f, 1.0f);
```

**Headroom:** without clamp, 8 loud voices can exceed ±1.0 and clip harshly (soft clip on master bus helps, but clamp here too).

---

## What the tests check

| Test | Validates |
|------|-----------|
| `two NOTE_ON` | Q1 — two slots active, frequencies assigned |
| `chord louder` | Q3 — mix sums voices |
| `partial NOTE_OFF` | Q2 — one note stops, others continue |
| `parse NOTE_OFF,freq` | GUI ↔ engine protocol |

---

## When you are done

1. `./build/test_lab08` all `ok`
2. Play chords in the GUI
3. Check off Lab 08 on [ROADMAP.md](../ROADMAP.md)

**Extensions:** [lab09](../lab09/README.md) waveforms · [lab10](../lab10/README.md) NEON · [lab11](../lab11/README.md) Core Audio

---

## Stretch

- Voice stealing fade (quick release before reassign)
- Per-voice pan in the stereo field
- Use Lab 10 SIMD batch inside each voice at Lab 08 scale
