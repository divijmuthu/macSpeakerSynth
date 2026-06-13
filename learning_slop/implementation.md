**Leaner summary of actual implementation and signal flow**

**Objectives**

- **Goal:** Build a minimal, low-latency C++ synth engine with a Python controller (see [plan.md](plan.md)).
- **Scope:** Prioritize a clear signal path, deterministic audio callback, and testable DSP primitives (oscillator, ADSR, biquad).

**Modern DSP Alignment**

- The design intentionally uses core, well-understood building blocks: phase-accumulator oscillators, ADSR envelopes, and biquad filters. These are foundational and map directly to embedded/real-time systems.
- Considerations to align with modern best-practices:
  - Anti-aliasing for band-limited waveforms (BLEP/BLIT) if you add hard-edged waves.
  - Use wavetables + interpolation or SIMD for CPU efficiency at scale.
  - Keep audio thread lock-free: no mallocs, no printf, avoid syscalls.
  - Optional: fixed-point math for constrained embedded targets.

**What to Learn First: The Core Audio Model**

- The audio callback is the real core of the project. It does not "play a whole song" at once; it repeatedly fills a small chunk of output samples called a buffer.
- A typical callback receives:
  - `outputBuffer`: interleaved float samples
  - `frameCount`: number of audio frames in this callback
  - `sampleRate`: e.g. 48000 Hz
- One frame usually means one time step for all channels. For mono, one frame = one sample. For stereo, one frame = two samples: left and right.
- The most important state you will carry between callbacks is not the whole waveform, but tiny DSP state such as:
  - current oscillator phase
  - envelope stage / level
  - filter history (x[n-1], x[n-2], y[n-1], y[n-2])

**Core Data Structures to Start With**

- `AudioState`: sample rate, master gain, voice count, and the current command queue.
- `Voice`: oscillator phase, frequency, waveform type, gain, envelope state.
- `EnvelopeState`: attack/decay/sustain/release parameters and current level.
- `BiquadState`: filter coefficients and delay history.

A minimal mental model is:

```cpp
struct Voice {
  float phase = 0.0f;
  float freqHz = 440.0f;
  float gain = 0.0f;
};

struct AudioState {
  float sampleRate = 48000.0f;
  Voice voice;
};
```

The callback then does this every block:

```cpp
for (int i = 0; i < frameCount; ++i) {
  float sample = generateSample(state.voice);
  output[2 * i + 0] = sample;
  output[2 * i + 1] = sample;
}
```

**Beginner Milestone**

1. Make one mono sine wave appear in the output buffer.
2. Make the phase advance by `freq / sampleRate` each sample.
3. Confirm the callback is filling one block at a time.
4. Only after that, add ADSR, filter, and IPC.

**Signal Flow (ASCII)**

```
Python UI (virtual keyboard, knobs)
  |
  | ZeroMQ PUB
  v
  Python -> [ZeroMQ] ---> C++ SUB -> LockFreeQueue
                                 |
                                 v
                         AudioCore (audio callback)
                                 |
                      +----------+-----------+
                      |                      |
               Voices[] (for each voice)     Master Mixer -> DAC
                      |                      |
  +-------------------+------------------+   |
  | Oscillator -> ADSR Envelope -> VCF  |   |
  |  (`Oscillator`)  (`Envelope`) (`Biquad`)|  |
  +---------------------------------------+   v
                                          Output buffer (interleaved samples)
```

**Lean Roadmap**

- Phase A: Build and run a single-voice 440Hz phase-accumulator sine in `AudioCore.cpp`.
- Phase B: Add `Envelope` (ADSR) and ensure no clicks on NoteOn/Off.
- Phase C: Wire Python -> ZMQ -> LockFreeQueue and map a simple `NOTE_ON` message to set frequency.
- Phase D: Add `Biquad` low-pass and a simple UI slider for cutoff.
- Phase E (opt): Add band-limited saw/square (BLEP) and simple polyphony.

**Implementation Style: Fill-in-the-Blank Modules**

- For each small module provide a skeleton file with TODO markers and unit tests:
  - `Oscillator.`* : implement `advancePhase()` and `renderSample()` (TODO: BLEP hook).
  - `Envelope.`* : implement state transitions in `noteOn()`/`noteOff()` and `process()` returns gain.
  - `Biquad.*` : provide `setCoeffs(cutoff, q)` and `process(sample)`.
- Tests: write tiny, fast unit tests in `tests-and-benchmarks/test_dsp.cpp` that drive each class deterministically.

**Critical System Decisions (pick and lock early)**

- Audio API: `miniaudio` (current) vs platform native — miniaudio simplifies cross-platform testing.
- IPC: ZeroMQ (current) vs simple sockets or shared memory — ZMQ is easy and robust for prototyping.
- Sample rate strategy: fixed (e.g. 48k) or adaptive — fix at start to simplify coefficient math.
- Voice mixing: float gaccumulation then clamp vs. per-voice gain normalization.
- Anti-aliasing approach: naive (no AA) for prototypes, BLEP/BLIT when adding saw/square.

**Terse File Roles**

- **[src/main.cpp](src/main.cpp)**: process startup, ZMQ setup, spawn audio engine.
- **[include/AudioCore.h](include/AudioCore.h)** & **[src/AudioCore.cpp](src/AudioCore.cpp)**: audio callback, voice management, buffer handling.
- **[include/LockFreeQueue.h](include/LockFreeQueue.h)**: bridge for control messages into audio thread.
- **[include/DSP_utilities/Oscillator.h](include/DSP_utilities/Oscillator.h)** & **[src/DSP_utilities/Oscillator.cpp](src/DSP_utilities/Oscillator.cpp)**: phase accumulator and waveform outputs.
- **[include/DSP_utilities/Envelope.h](include/DSP_utilities/Envelope.h)** & **[src/DSP_utilities/Envelope.cpp](src/DSP_utilities/Envelope.cpp)**: ADSR FSM.
- **[include/DSP_utilities/Biquad.h](include/DSP_utilities/Biquad.h)** & **[src/DSP_utilities/Biquad.cpp](src/DSP_utilities/Biquad.cpp)**: filter math and coefficient updates.
- **[py_interface/main.py](py_interface/main.py)**: minimal UI, ZeroMQ publisher for control events.
- **[tests-and-benchmarks/test_dsp.cpp](tests-and-benchmarks/test_dsp.cpp)**: unit tests for primitives.

