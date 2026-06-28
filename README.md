# macSpeakerSynth 

Real-time audio synthesizer with a C++ audio engine and a Python UI.

## Current implementation

- C++17 synth engine running at 48 kHz.
- Python/Tkinter controller that sends live note/control messages over ZeroMQ.
- Core DSP blocks:
  - Oscillator with `Sine`, `Saw`, `Square`, `Triangle` --> Provides underlying waveforms computed via phase progression.
  - ADSR envelope --> Prevents sudden clicks/jumps, allows gradual ring up/down for hardware, enables tuning of pitch development.
  - Filters: Biquad LPF + State Variable Filter (LP/HP/BP) --> Block or attenuate certain frequencies, implemented via classic biquad + coefficients and SVF which computes the required samples for all three together.
  - Master effects: delay (feedback + wet/dry) and soft clip --> Produces lasting, recurring tones.
- 8-voice polyphony with voice allocation/stealing.
- Lock-free SPSC queue between control thread and audio callback.
- Runtime options:
  - SIMD toggle for oscillator batch path (offers potential for performance gains by parallelizing repetitive waveform calculations on a vector of phase arguments for 4 samples at a time).
  - Audio backend selection on macOS (`Core Audio` or `miniaudio`).

## Architecture (3 layers)

1. **UI layer (Python)**: keyboard/controls emit messages (`NOTE_ON`, `CUTOFF`, `WAVEFORM`, etc.).
2. **Control layer (C++ non-audio threads)**: ZMQ subscriber parses messages and pushes them into a lock-free queue.
3. **Audio layer (C++ callback thread)**: drains queue, updates voice/filter/effect state, renders output samples to the active backend.

This keeps the audio callback free from blocking work and cross-thread locks.

## Dependencies

- macOS (primary target)
- CMake 3.16+
- C++17 compiler (Apple Clang works)
- Python 3.9+
- ZeroMQ (`brew install zeromq`)



## Run (recommended)

From repo root:

```bash
./run.sh
```

What it does:

- builds `race_synth` if needed,
- creates `.venv` and installs Python deps if missing,
- launches engine + UI in separate Terminal windows on macOS.

Useful options:

```bash
./run.sh --rebuild
./run.sh --backend coreaudio
./run.sh --backend miniaudio
./run.sh --inline
```



## Manual build and run



### 1) Configure and build

```bash
cmake -S . -B build
cmake --build build --target race_synth
```



### 2) Start the C++ engine (Terminal A)

```bash
./build/race_synth
```



### 3) Start the Python UI (Terminal B)

```bash
python3 -m venv .venv
./.venv/bin/pip install -r py_interface/requirements.txt
./.venv/bin/python py_interface/main.py
```



## Build-time options

```bash
cmake -S . -B build \
  -DRACE_USE_COREAUDIO=ON \
  -DRACE_ENABLE_NEON=ON
```

- `RACE_USE_COREAUDIO`: include native macOS Core Audio backend.
- `RACE_ENABLE_NEON`: enable ARM NEON oscillator batch path when supported.



## Runtime backend selection (macOS)

```bash
RACE_AUDIO_BACKEND=coreaudio ./build/race_synth
RACE_AUDIO_BACKEND=miniaudio ./build/race_synth
```

Or use `./run.sh --backend ...`.

## Tests and benchmark

```bash
cmake --build build --target test_lab09
./build/test_lab09

cmake --build build --target benchmark_neon
./build/benchmark_neon
```



## Repo layout

```text
include/                 headers (AudioCore, DSP blocks, control messages)
src/                     engine, platform backends, DSP implementations
py_interface/            Python UI + ZMQ client
tests-and-benchmarks/    tests and benchmark target(s)
run.sh                   one-command launcher
```

