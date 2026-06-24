# Lab 11 — Assignment: Core Audio Backend Deep Dive

> Status note: see [CURRENT_STATE.md](../CURRENT_STATE.md) for current behavior.
> Architecture summary: [INTERVIEW_READY_RUNDOWN.md](../INTERVIEW_READY_RUNDOWN.md).

**Goal:** Explain and verify how the project uses macOS Core Audio while preserving a backend-agnostic DSP core.

---

## Assignment outcomes

By the end you should be able to answer:

1. Why introduce an `AudioOutput` abstraction at all?
2. How does `CoreAudioOutput.mm` connect callback audio into `AudioCore::renderBlock()`?
3. How can you force fallback backend and compare behavior?

---

## Part A — Read backend abstraction

Focus files:

- `include/platform/AudioOutput.h`
- `src/platform/MiniaudioOutput.cpp`
- `src/platform/CoreAudioOutput.mm`
- `src/AudioCore.cpp`
- `CMakeLists.txt` (`RACE_USE_COREAUDIO`)

Task:

- Sketch a diagram showing:
  - backend selection at build/runtime,
  - callback path into `renderBlock`,
  - where platform-specific code ends and engine code begins.

---

## Part B — Build both backend modes

### Core Audio mode (default on Apple)

```bash
cmake -S . -B build-core -DRACE_USE_COREAUDIO=ON
cmake --build build-core
./build-core/race_synth
```

### Force miniaudio fallback

```bash
cmake -S . -B build-mini -DRACE_USE_COREAUDIO=OFF
cmake --build build-mini
./build-mini/race_synth
```

Task:

- Confirm which backend starts by reading runtime print:
  - `Audio backend: Core Audio (macOS HAL)` or fallback.

### Runtime switch without rebuild (new)

When both backends are compiled (Apple + `RACE_USE_COREAUDIO=ON`), you can choose at launch:

```bash
RACE_AUDIO_BACKEND=coreaudio ./build-core/race_synth
RACE_AUDIO_BACKEND=miniaudio ./build-core/race_synth
```

This makes A/B backend checks faster during tuning.

---

## Part C — Realtime safety audit

Check that backend code obeys callback rules:

- no blocking calls in callback,
- no dynamic allocations in callback,
- no locks around per-sample render path.

Write a short note with any caveats you find.

---

## Canonical reference implementation

Lab 11 backend abstraction and Core Audio wrapper are already implemented in this repo.
Your assignment is to validate and explain the design, then compare backend behavior.

---

## Completion checklist

- [ ] I can explain why `AudioOutput` exists.
- [ ] I can build/run Core Audio and fallback modes.
- [ ] I can describe callback flow and realtime safety constraints.

Return to [ROADMAP.md](../ROADMAP.md).
