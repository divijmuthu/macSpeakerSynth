# macSpeakerSynth (RACE)

A **learn-by-building** real-time synthesizer: C++ audio engine + Python controller.

**Start here:** [learning/00_START_HERE.md](learning/00_START_HERE.md)

**Roadmap (labs → finished synth):** [learning/ROADMAP.md](learning/ROADMAP.md)
  
**Current behavior snapshot:** [learning/CURRENT_STATE.md](learning/CURRENT_STATE.md)  
**Reference implementation checklist:** [learning/REFERENCE_IMPLEMENTATIONS.md](learning/REFERENCE_IMPLEMENTATIONS.md)
  
**Interview-ready system rundown:** [learning/INTERVIEW_READY_RUNDOWN.md](learning/INTERVIEW_READY_RUNDOWN.md)  
**Performance tracking (SIMD/CoreAudio):** [learning/PERFORMANCE_TRACKING.md](learning/PERFORMANCE_TRACKING.md)

**Abbreviations & jargon:** [learning/GLOSSARY.md](learning/GLOSSARY.md)

**Current lab:** [learning/lab09/README.md](learning/lab09/README.md) — Waveforms

---

## Current step

**Lab 09** → [lab09/README.md](lab09/README.md)

---

## Quick start (Lab 09)

```bash
cmake --build build && ./build/test_lab09
./build/race_synth
python py_interface/main.py     # keyboard: A S D F = C major chord
```

---

## Teaching structure

| Lab | Topic |
|-----|--------|
| 01 | Phase accumulator + sine ✓ |
| 02 | ADSR envelope + Voice ✓ |
| 03 | miniaudio callback ✓ |
| 04 | ZMQ + lock-free queue ✓ |
| 05 | Biquad LPF ✓ |
| 06 | State-variable filter ✓ |
| 07 | Delay + soft saturation ✓ |
| 08 | Polyphony ✓ |
| 09 | Waveforms *(active)* |

Full curriculum (DSP + C++ keywords): [learning/ROADMAP.md](learning/ROADMAP.md)

Implementation notes and signal-flow diagrams: [learning_slop/implementation.md](learning_slop/implementation.md)

---

## Original design doc

The full architecture spec (VCO, VCF, CMake layout, phases) is preserved at the bottom of this file for reference.

---

Design Doc:
Integrating a synthesizer is a brilliant pivot. In fact, a software synthesizer (soft-synth) is arguably the *perfect* project to learn applied DSP and systems engineering. It forces you to deal with continuous audio generation, state machines, and real-time parameter modulation.

To keep the architecture highly performant, we won't have Python play the system audio. **Python will act as the "MIDI Controller" and UI.** When you press a key or move a slider in Python, it will send a tiny control message (e.g., `["NOTE_ON", 440.0]`) via ZeroMQ to your C++engine. The C++ engine, running the continuous audio callback, will instantly generate the math for that 440Hz wave and push it to your MacBook's speakers.

This setup bridges theoretical signals and systems concepts—like analyzing Linear Time-Invariant (LTI) systems and frequency responses—directly into high-performance C++ code. This is exactly the kind of architecture used in industry, from pro-audio plugins to the embedded DSP systems managing chimes, active noise cancellation, and spatial audio in modern electric vehicles.

Here is the updated design document incorporating the synthesizer.

---

## Project Design Document: Real-Time Audio Synthesizer & Control Engine (RACE)

### 1 Project Objective

To build a low-latency, real-time software synthesizer and audio processing pipeline in C++. The C++ engine will generate waveforms from scratch and apply custom DSP filters. A separate Python process will act as the user interface, sending real-time control parameters and "Note On/Off" triggers to the C++ engine over an IPC bridge.

### 2 Core Architecture

The system consists of a "headless" C++ audio engine and a Python control GUI.

**Component A: The C++ Synthesis Engine (The Muscle)**

- **Role:** Runs the high-priority `miniaudio` callback. It manages an array of "Voices" (oscillators), processes their volume envelopes, and routes them through mathematical filters before outputting to the DAC.
- **Constraints:** No blocking operations, no `malloc` during the audio loop. State variables (like oscillator phase) must update with extreme precision.

**Component B: The Python Controller (The Brain)**

- **Role:** Provides a graphical interface (e.g., a virtual piano keyboard and knobs for filter cutoffs).
- **Action:** Converts user inputs into discrete control messages and publishes them to the C++ engine via a ZeroMQ socket.

### 3 DSP Algorithm Focus (From Scratch)

You will implement the foundational building blocks of a subtractive synthesizer entirely through math.

- **Voltage Controlled Oscillators (VCO):**
  - *Concept:* Generating raw waveforms (Sine, Square, Sawtooth, Triangle).
  - *Implementation:* You will use a **Phase Accumulator**. Instead of calculating an expensive `sin()` function for every sample, you increment a phase variable based on the sample rate and frequency, wrapping it at $2\pi$.
- **Envelope Generators (ADSR):**
  - *Concept:* Shaping the amplitude of the sound over time so it doesn't just abruptly click on and off.
  - *Implementation:* A Finite State Machine (FSM) running inside the audio callback that multiplies the oscillator output by a dynamically changing gain value ($0.0$ to $1.0$) based on Attack, Decay, Sustain, and Release times.
- **Voltage Controlled Filters (VCF):**
  - *Concept:* Using your custom Biquad filters to "sculpt" the raw oscillator waves (subtractive synthesis).
  - *Implementation:* Sweeping the cutoff frequency of a Low-Pass Biquad filter using the difference equation:
  $$y[n] = \frac{b_0x[n] + b_1x[n-1] + b_2x[n-2] - a_1y[n-1] - a_2y[n-2]}{a_0}$$
  You will need to calculate the $a$ and $b$ coefficients dynamically whenever Python sends a new cutoff frequency.

### 4 CMake Project Structure

This structure ensures your code is modular, separating the platform-specific audio code from your pure DSP math.

```text
race_synth_engine/
├── CMakeLists.txt          
├── src/
│   ├── main.cpp            # Initializes ZeroMQ SUB, starts miniaudio
│   ├── AudioCore.cpp     # The miniaudio callback
│   └── DSP_utilities/
│       ├── Oscillator.cpp  # Phase accumulation math
│       ├── Envelope.cpp    # ADSR state machine
│       └── Biquad.cpp      # Filter math
├── include/                
│   ├── AudioCore.h
│   ├── LockFreeQueue.h     # To safely receive Python messages
│   └── DSP_utilities/
│       ├── Oscillator.h
│       ├── Envelope.h
│       └── Biquad.h
├── tests-and-benchmarks/                  # Unit tests for your DSP math
│   └── test_dsp.cpp
└── py_interface/
    ├── main.py             # UI, Virtual Keyboard, ZMQ PUB
    └── requirements.txt
```

### 5 Phase-by-Phase Execution Plan

- **Phase 1: The Continuous Beep.** Get the CMake project building. Hardcode a 440Hz sine wave phase accumulator in the C++ callback and output it to the speakers
- **Phase 2: The ADSR Envelope.** Implement the Envelope class. Trigger it with a hardcoded timer to ensure the beep smoothly fades in and out without popping.
- **Phase 3: The Python Bridge.** Implement the ZeroMQ Lock-Free Queue. Send a frequency from a Python script (e.g., 880Hz) and have the C++ sine wave instantly change pitch.
- **Phase 4: Complex Waves & Filters.** Add Sawtooth and Square wave generation. Route the oscillator output through your Biquad Low-Pass filter. Add a slider in Python to control the filter's cutoff frequency.
- **Phase 5: Polyphony (Advanced).** Upgrade the C++ engine to handle an array of oscillators simultaneously so you can play chords, mixing their outputs together before sending them to the DAC.

---

This is an incredibly robust system to build. It will force you to deeply understand how data flows in a real-time system and how to translate discrete-time math into clean C++ classes.

Would you like to start by sketching out the C++ class structure for the `Oscillator` and `Phase Accumulator` to see how we generate a sine wave without blowing up the CPU?
