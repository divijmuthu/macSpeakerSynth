# Interview-Ready Implementation Rundown

Use this as a concise walkthrough of what you built and why the architecture makes sense.

## Project in one sentence

RACE is a real-time C++ synth engine with a Python control UI: controls arrive over ZMQ, are queued lock-free, and are consumed by the audio callback which renders voices and effects with no blocking work on the realtime path.

## End-to-end data flow

1. `py_interface/main.py` UI emits control messages (`NOTE_ON`, `CUTOFF`, `WAVEFORM`, `GAIN`, etc.).
2. `src/main.cpp` runs a ZMQ subscriber thread and pushes parsed `ControlMessage` values into `LockFreeQueue`.
3. `AudioCore::renderBlock()` (called by audio backend callback) drains that queue and updates audio-thread-owned DSP state.
4. Voices render per sample, are mixed polyphonically, then pass through master bus effects and output to DAC.
5. Scope snapshots are published on a second ZMQ PUB endpoint for UI visualization.

## Why this threading model is correct

- Audio callback thread does not lock, allocate, or wait.
- Control-plane work is offloaded to non-audio threads.
- Queue boundary isolates timing jitter from UI/network from realtime audio.
- Voice state is mutated only on the audio thread, avoiding cross-thread races in DSP objects.

## DSP pipeline (current)

Per voice:

`Oscillator (sine/saw/square/triangle) -> SVF filter -> ADSR gain`

Global:

`Poly mix (8 voices, headroom strategy + smoothing) -> master gain -> optional delay wet/dry -> optional soft clip`

## Polyphony decisions

- Voice pool: fixed `std::array<VoiceSlot, 8>`.
- Allocation: retrigger same pitch if active, else free slot, else oldest-slot steal.
- Release: pitch-targeted `NOTE_OFF,<hz>` or global `NOTE_OFF,0`.
- Mix management: active-voice-aware gain + slew to reduce pop on 1->N transitions.

## Lab 10 (SIMD / NEON) implementation summary

- Added `SimdOscillator` batch API (`oscillatorRenderBatch`) that processes in chunks.
- Build-gated by `RACE_NEON` on arm64/aarch64.
- Benchmark target compares scalar `nextSample()` and batch path throughput.
- Current use is benchmark/utility path (not yet fully threaded into every voice render site).

## Lab 11 (Core Audio) implementation summary

- Introduced backend abstraction: `AudioOutput` with `start() / stop() / backendName()`.
- Implemented:
  - `MiniaudioOutput` for cross-platform fallback.
  - `CoreAudioOutput` using AudioUnit DefaultOutput on macOS.
- `AudioCore` becomes backend-agnostic and hands off render callback to selected backend.
- Added runtime backend override via env var on macOS builds:
  - `RACE_AUDIO_BACKEND=coreaudio|miniaudio`.
- CMake switches:
  - `RACE_USE_COREAUDIO=ON` (default on Apple)
  - `RACE_USE_COREAUDIO=OFF` to force miniaudio fallback on macOS.

## Realtime correctness talking points (interview)

- Lock-free message handoff between control and audio threads.
- No dynamic memory on callback path.
- Deterministic sample processing loop.
- Explicit handling of headroom/clipping in polyphonic sum.
- Platform backend isolated behind interface, preserving DSP core portability.

## Known quality tradeoffs (honest + strong)

- Naive saw/square are intentionally educational and alias at high pitch.
- SIMD path currently demonstrates acceleration infrastructure; further integration can improve net gains.
- Musical polish (anti-aliasing, limiter flavor, voice-steal fades) is iterative and expected for synth projects.
