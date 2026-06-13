# Lab 03 — Audio callback & first beep

**Goal:** Wire your Lab 02 `Voice` into a **miniaudio** callback and hear a 440 Hz tone from your Mac speakers.

**Prerequisites:** Lab 02 green (`./build/test_lab02`).

**Read:** [ROADMAP.md](../ROADMAP.md) — full curriculum including threading, filters, and effects.

---

## What Lab 03 actually contributes (read this first)

Labs 01–02 built **DSP math** — formulas that turn numbers into waveform samples. Lab 03 adds the **real-time delivery layer**: the machinery that calls your math 48,000 times per second and ships the result to hardware.

Think of the final synth as three layers:

```text
┌─────────────────────────────────────────────────────────────┐
│  LAYER 3 — Control (Lab 04+)                                │
│  Python UI, ZeroMQ, lock-free queue                         │
│  "User pressed A4" → change frequency                       │
└───────────────────────────┬─────────────────────────────────┘
                            │  (not built yet)
┌───────────────────────────▼─────────────────────────────────┐
│  LAYER 2 — Real-time engine  ◄── YOU ARE HERE (Lab 03)      │
│  AudioCore: callback, buffer fill, device I/O               │
│  "Play the next 256 frames NOW" → call Voice repeatedly     │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  LAYER 1 — DSP kernels (Labs 01–02, extended in 05–08)      │
│  Oscillator, Envelope, Voice, filters, effects              │
│  "Given my current state, what is one sample?"              │
└─────────────────────────────────────────────────────────────┘
```

**Lab 03 does not add new audio math.** It answers a systems question: *how does a C++ program continuously stream samples to speakers without stuttering?*

| You write today | Permanent role in the finished synth |
|-----------------|--------------------------------------|
| `renderBlock()` | The inner loop every future lab extends (queue drain, mixing, metering) |
| `audioDataCallback` | The bridge from miniaudio's C callback to your C++ class |
| `start()` / `stop()` | Device lifecycle — open DAC, register callback, tear down cleanly |
| `main()` sleep loop | Keeps the process alive while the **audio thread** does the work |
| `pUserData = this` | Classic pattern: pass `AudioCore*` into a C callback without globals |

After Lab 03, `./build/race_synth` is a **headless real-time audio process** — the same shape as a DAW plugin or game audio engine, just with a hardcoded note instead of a UI.

---

## Multithreading: yes, starting now

Lab 03 introduces **two threads**:

```text
  MAIN THREAD (your main.cpp)              AUDIO THREAD (miniaudio)
  ─────────────────────────────            ──────────────────────────────
  AudioCore engine;
  engine.voice().noteOn();
  engine.start();  ──registers──►          [high-priority thread spawned]
       │                                   every ~5 ms:
       │                                     audioDataCallback()
       ▼                                       → renderBlock()
  while (true) sleep(100ms);                    → voice_.nextSample() × N
  // process stays alive                         → write to hardware buffer
```

| Thread | Job in Lab 03 | Must not… |
|--------|---------------|-----------|
| **Main** | Setup, `noteOn()`, `start()`, keep process alive | Assume it runs the audio loop |
| **Audio** | Fill each output buffer before the DAC needs it | Block, allocate, lock, print |

This is **producer/consumer** thinking, but the "producer" of samples is the audio thread itself — it consumes *time* (deadline every buffer) and produces *samples*.

**Shared state in Lab 03:** one `Voice` inside `AudioCore`. Only the audio thread touches it during playback — no race yet. That changes in **Lab 04** when a ZMQ thread writes control messages while the audio thread reads them (lock-free queue + atomics).

**C++ keywords you touch in Lab 03:**

| Concept | Where |
|---------|--------|
| `static_cast` | C callback → `AudioCore*` via `pUserData` |
| RAII | `AudioCore` destructor calls `stop()` |
| Separation of concerns | DSP in `Voice`, I/O in `AudioCore` |
| Real-time safety | No heap/log/locks in `renderBlock` |
| Thread (implicit) | miniaudio spawns; you do not create `std::thread` yet |

Lab 04 adds explicit `std::thread`, atomics, and lock-free structures — see [ROADMAP.md](../ROADMAP.md).

---

## What changes today (summary)

| Before (Lab 02) | After (Lab 03) |
|-----------------|----------------|
| `voice.nextSample()` in unit tests | Same call, inside `AudioCore::renderBlock()` |
| No hardware | miniaudio opens speakers, calls your code ~187×/sec |
| Tests only | Tests **plus** `./build/race_synth` — use your ears |

Nothing in `Oscillator`, `Envelope`, or `Voice` gets rewritten. You add the **delivery layer**.

---

## End-to-end data flow (one buffer)

This is what happens in the ~5 ms window between callback invocations:

```text
  1. OS DAC buffer running low
           │
           ▼
  2. miniaudio audio thread wakes up
           │
           ▼
  3. audioDataCallback(device, output, …, frameCount=256)
           │
           ├─ device->pUserData  ──►  AudioCore*  (Q1)
           │
           ▼
  4. renderBlock(output, 256)  (Q2)
           │
           for i = 0 .. 255:
             s = voice_.nextSample()     ← Lab 02: osc × envelope
             output[2i+0] = s            ← left
             output[2i+1] = s            ← right
           │
           ▼
  5. miniaudio copies buffer → CoreAudio → Mac speakers
```

**Sample rate math:** at 48 kHz with 256-frame buffers, the callback runs ~187 times/sec. Each call must finish in well under 5 ms or you get **underruns** (crackling).

---

## Files you edit

| File | Questions | Contributes |
|------|-----------|-------------|
| `src/AudioCore.cpp` | Q1, Q2, Q3 | Real-time engine — stays forever |
| `src/main.cpp` | Q4, Q5 | Process entry; Lab 04 adds ZMQ thread here |

`include/AudioCore.h` is provided — skim it.

---

## Build

miniaudio is vendored under `third_party/miniaudio/` (no network needed).

```bash
cmake -S . -B build
cmake --build build
./build/test_lab03      # offline — no speakers needed
./build/race_synth      # hear the beep; Ctrl+C to quit
```

---

## New concepts

### The audio callback

The OS / miniaudio repeatedly asks: *“give me the next N frames.”* One **frame** = one stereo pair (L + R). A typical **buffer** is 256 frames = 512 float samples interleaved.

```text
  time ──────────────────────────────────────────────►

  callback #1          callback #2          callback #3
  [256 frames]         [256 frames]         [256 frames]
       │                    │                    │
       └─ renderBlock()     └─ renderBlock()     └─ …
```

Your job in `renderBlock` is identical to what you did in tests — just 256 times per call instead of once.

### Interleaved stereo layout

```text
  index:  0    1    2    3    4    5
          L0   R0   L1   R1   L2   R2
```

Same mono signal in L and R: `out[2*i+0] = out[2*i+1] = s`.

### Real-time rules (enforce from now on)

Inside `renderBlock` / the miniaudio callback:

| Allowed | Not allowed |
|---------|-------------|
| Read/write `Voice` state | `malloc` / `new` |
| `voice_.nextSample()` | `printf`, logging |
| Simple loops, arithmetic | Locks, sleeping, file I/O |

`start()` runs **once** on the main thread — allocating `ma_device` there is fine. The callback must stay lean.

**Why?** The audio thread has a **deadline**. If it waits on a lock held by main, or stalls in `printf`, the next buffer is late → glitch.

### miniaudio roles

| Piece | Purpose |
|-------|---------|
| `ma_device_config` | Describe playback: format, channels, sample rate |
| `dataCallback` | Function pointer miniaudio calls each buffer |
| `pUserData` | Back-pointer to your `AudioCore` (`this`) |
| `ma_device_init` / `ma_device_start` | Open speakers |

`MINIAUDIO_IMPLEMENTATION` lives in **one** `.cpp` only (`AudioCore.cpp`).

---

## Concept check

1. Who calls `renderBlock` — your `main()` or miniaudio?  
   *(miniaudio, on a high-priority audio thread)*

2. Why call `noteOn()` **before** `start()`?  
   *(so the first buffer already has the attack ramp — avoids a full-volume pop)*

3. If `frameCount == 256`, how many floats do you write to `output`?  
   *(512 — 256 × 2 channels)*

4. Is Lab 03 multithreaded?  
   *(Yes — main thread + miniaudio audio thread. You do not spawn threads yourself until Lab 04.)*

5. Why is `renderBlock` separate from `audioDataCallback`?  
   *(Testability — `test_lab03` calls `renderBlock` directly without opening speakers.)*

---

## Question 1 — miniaudio callback glue

**File:** `src/AudioCore.cpp` — function `audioDataCallback`  
**TODO:** `Lab 03, Q1`

Cast `device->pUserData` back to `AudioCore*` and forward to `renderBlock`.

This C function exists because miniaudio expects a plain function pointer — it cannot be a C++ member function directly.

---

## Question 2 — `renderBlock` loop

**TODO:** `Lab 03, Q2`

```cpp
for (std::uint32_t i = 0; i < frameCount; ++i) {
    const float s = voice_.nextSample();
    output[2 * i + 0] = s;
    output[2 * i + 1] = s;
}
```

This is the **production version** of the loop you wrote conceptually in Lab 02’s README. Lab 04 adds one line *before* this loop; Lab 05 changes what happens *inside* `Voice::nextSample()`.

---

## Question 3 — `start()` device setup

**TODO:** `Lab 03, Q3`

After building `config` from the TODO comments:

1. `ma_device_init(nullptr, &config, device_)` — `nullptr` = default output device (Mac speakers).
2. On failure: `delete device_; device_ = nullptr; return false;`
3. `ma_device_start(device_)` — on failure: `ma_device_uninit(device_); delete device_; device_ = nullptr; return false;`
4. Set `running_ = true` only after both succeed.

---

## Question 4 & 5 — `main.cpp`

**Q4:** `engine.voice().noteOn();` before starting audio.

**Q5:** Check `engine.start()` return value; exit with error message if false.

The `for (;;)` sleep loop is intentional — it keeps `main` alive. Closing the process would tear down audio even though the callback runs elsewhere.

---

## Verify

### Step 1 — unit tests (no hardware)

```bash
./build/test_lab03
```

All four checks should print `ok`. Tests call `renderBlock` directly on the main thread — same code path as the audio thread, no device needed.

### Step 2 — ears

```bash
./build/race_synth
```

You should hear a **440 Hz** tone that **fades in** (attack ~20 ms), holds, and keeps playing until Ctrl+C.

| Symptom | Likely cause |
|---------|----------------|
| Silence | Q2 loop empty, or Q4 noteOn missing |
| Loud pop at start | noteOn after start(), or envelope bypassed |
| Crackling | callback too slow (unlikely at one voice) |
| `Failed to start audio device` | permissions / headphones routing — check System Settings → Sound |

---

## Where this lands next

| Lab | Adds to what you built today |
|-----|------------------------------|
| **04** | `std::thread` for ZMQ + lock-free queue; two threads touch shared state safely |
| **05** | Biquad low-pass inside `Voice` |
| **06** | State-variable filter (smoother cutoff sweeps) |
| **07** | Delay + soft saturation (time-based + nonlinear DSP) |
| **08** | Polyphony — array of voices mixed in `renderBlock` |

See the full curriculum map: [ROADMAP.md](../ROADMAP.md).

---

## When you are done

1. Check off Lab 03 on [ROADMAP.md](../ROADMAP.md).
2. Optional Lab 02 polish: add `case EnvelopeStage::Sustain: break;` and `break;` after Decay in `Envelope.cpp`.

Next: [lab04/README.md](../lab04/README.md)
