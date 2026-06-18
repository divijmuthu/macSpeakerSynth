# Lab 11 — macOS Core Audio backend

**Goal:** Talk to the Mac speakers through **Apple's HAL** (Hardware Abstraction Layer) instead of only miniaudio.

---

## Two backends

| Backend | When | API |
|---------|------|-----|
| **Core Audio** | macOS, `RACE_USE_COREAUDIO=ON` (default) | `AudioUnit` DefaultOutput |
| **miniaudio** | fallback / other platforms | portable callback |

`AudioOutput::createDefault()` picks at compile time. `race_synth` prints which backend started:

```text
Audio backend: Core Audio (macOS HAL)
```

---

## Architecture

```text
  Core Audio thread          AudioCore::renderBlock()
        │                              │
        └──── AURenderCallback ────────┘
```

Same `renderBlock` loop — only the **device wrapper** changed (Lab 03 callback idea, platform-specific shell).

---

## CMake

```bash
cmake -S . -B build                    # Core Audio on Apple
cmake -S . -B build -DRACE_USE_COREAUDIO=OFF   # force miniaudio on Mac
```

Links: `AudioToolbox`, `AudioUnit`, `CoreAudio`.

---

## Files

| File | Role |
|------|------|
| `include/platform/AudioOutput.h` | abstract `start` / `stop` |
| `src/platform/CoreAudioOutput.mm` | Objective-C++ AudioUnit |
| `src/platform/MiniaudioOutput.cpp` | cross-platform fallback |

Real-time rules unchanged: **no locks, no alloc** inside `renderBlock`.

---

## Stretch

- Device picker (aggregate device, Bluetooth latency)
- `AudioObject` property listeners for sample-rate change
- Compare callback jitter: Core Audio vs miniaudio

Return to [ROADMAP.md](../ROADMAP.md).
