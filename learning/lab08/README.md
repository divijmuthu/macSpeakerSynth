# Lab 08 — Polyphony & voice mixing

**Unlock after:** Lab 07.

**Goal:** Play **chords** — multiple `Voice` instances mixed in `renderBlock`.

---

## What this lab contributes

| DSP | C++ |
|-----|-----|
| Summing voices, headroom, soft clip optional | `std::array<Voice, kMaxVoices>` |
| Voice allocation on NOTE_ON | Find free slot / steal oldest |
| NOTE_OFF targets one voice | Index or round-robin note map |

```cpp
float mix = 0.0f;
for (auto& v : voices_) {
    if (v.isActive()) mix += v.nextSample();
}
mix = std::clamp(mix, -1.0f, 1.0f);
```

Same threading model as Lab 04 — all voice mixing stays on the **audio thread**.

See [ROADMAP.md](../ROADMAP.md).
