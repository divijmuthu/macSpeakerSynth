# Glossary — abbreviations & ideas

Look up terms here when a lab or comment uses jargon you do not recognize. Entries are ordered roughly by when they appear in the labs.

---

## Audio basics

| Term | Stands for / means | One-line explanation |
|------|--------------------|----------------------|
| **Sample** | — | One number representing air pressure at one instant. A stream of samples *is* digital audio. |
| **Sample rate** | — | How many samples per second (e.g. **48000 Hz** = 48,000 samples/sec). Higher = better high-frequency detail; 44.1k and 48k are standard. |
| **Hz** | Hertz | Cycles per second. 440 Hz = 440 sine waves per second (concert A). |
| **Frame** | — | One time step for all channels. Mono: 1 sample/frame. Stereo: 2 samples/frame (L, R). |
| **Buffer / block** | — | A chunk of samples the audio system asks you to fill in one go (e.g. 256 frames). The **callback** runs once per buffer. |
| **Callback** | — | A function the OS/audio library calls repeatedly: “fill this buffer with the next N samples.” Your synth’s heart. |
| **DAC** | Digital-to-Analog Converter | Hardware that turns sample numbers into speaker movement. You never talk to it directly — the audio API does. |
| **Interleaved** | — | Stereo layout: L, R, L, R, … in one array. Index `2*i` = left, `2*i+1` = right. |

---

## Synthesis & DSP

| Term | Stands for / means | One-line explanation |
|------|--------------------|----------------------|
| **DSP** | Digital Signal Processing | Math on sample streams: oscillators, filters, effects. |
| **Soft-synth** | Software synthesizer | A program that *generates* sound from math instead of playing recorded files. |
| **VCO** | Voltage Controlled Oscillator | Analog synth term. Here: code that outputs a repeating waveform at a set **frequency**. |
| **Phase accumulator** | — | Instead of calling `sin()` with absolute time, keep a **phase** angle and add a small step each sample. Cheap and standard. |
| **Phase** | — | Where you are inside one cycle of a wave, usually in radians (0 → 2π → wrap). |
| **ADSR** | Attack, Decay, Sustain, Release | Four stages shaping **volume over time** after you press/release a key. Stops clicks and sounds “musical.” |
| **Envelope** | — | The time-varying gain (0.0–1.0) from ADSR. Multiplied against the raw oscillator output. |
| **VCF** | Voltage Controlled Filter | Analog term. Here: a filter whose **cutoff frequency** you can change (Lab 05). |
| **Biquad** | — | Second-order IIR filter — current + 2 past inputs + 2 past outputs. Lab 05. |
| **SVF / TPT** | State-Variable Filter / Trapezoidal | Multimode filter (LP/HP/BP); smoother when modulating cutoff. Lab 06. |
| **IIR** | Infinite Impulse Response | Filter using feedback (past outputs) — biquad, SVF. |
| **FIR** | Finite Impulse Response | Filter using only past inputs — delay line taps (Lab 07). |
| **Wet/dry mix** | — | Blend processed (effect) and dry (original) signal. Lab 07. |
| **Soft clipping** | — | Nonlinear saturation past ±1 — analog warmth, limits harsh peaks. Lab 07. |
| **Delay line** | — | Circular buffer of past samples for echo. Lab 07. |
| **LTI** | Linear Time-Invariant | System whose output scales with input and whose behavior does not change over time. Biquads (with fixed coeffs) are LTI. |
| **Subtractive synthesis** | — | Start with bright raw waves (saw/square), then **subtract** harmonics with a low-pass filter. Classic synth recipe. |
| **Polyphony** | — | Many notes at once — one **voice** (oscillator + envelope) per active note. |
| **Voice** | — | One sounding note’s state: phase, frequency, envelope stage, etc. |
| **BLEP / BLIT** | Band-Limited Step / Impulse | Advanced anti-aliasing for harsh waves (Lab 06 optional). Skip until basic sine + ADSR work. |
| **Anti-aliasing** | — | Stopping “digital crunch” when waveforms have sharp edges above Nyquist. Naive square/saw alias; sine does not. |

---

## Systems & architecture

| Term | Stands for / means | One-line explanation |
|------|--------------------|----------------------|
| **RACE** | Real-time Audio synthesizer & Control Engine | This project’s name in the design doc. |
| **Real-time** | — | Deadlines matter: the callback must finish before the next buffer is due, or you get **glitches** (clicks/dropouts). |
| **IPC** | Inter-Process Communication | How Python (UI) talks to C++ (audio) — here **ZeroMQ** messages. |
| **ZMQ / ZeroMQ** | Zero Message Queue | Library for sending small messages between processes over TCP or in-process. Python publishes; C++ subscribes. |
| **Lock-free queue** | — | A queue safe for one producer (Python thread) and one consumer (audio thread) without mutexes in the hot path. |
| **FSM** | Finite State Machine | Code with explicit **states** (e.g. Attack, Decay) and rules for moving between them. ADSR is an FSM. |
| **miniaudio** | — | Single-header C library that opens your speakers and calls your callback. Used from Lab 03. |
| **Underrun / dropout** | — | Audio buffer not filled in time → crackle or silence. Real-time deadline missed. |
| **Control rate** | — | Slow updates (UI, MIDI) — tens–hundreds per second. Lab 04+. |
| **Audio rate** | — | Per-sample processing at 48 kHz. Your callback and DSP kernels. |

---

## Threading & concurrency (Lab 03+)

| Term | Stands for / means | One-line explanation |
|------|--------------------|----------------------|
| **Audio thread** | — | High-priority thread that runs your callback. Lab 03 (miniaudio spawns it). |
| **Main thread** | — | Runs `main()`, setup, keeps process alive. Lab 03+. |
| **SPSC queue** | Single-Producer Single-Consumer | Lock-free queue: one thread pushes, one pops. Lab 04 `LockFreeQueue`. |
| **Lock-free** | — | Progress without mutexes — critical for audio side never blocking on UI. |
| **`std::atomic`** | — | Safe flags/parameters across threads without locks. Lab 04+. |
| **Data race** | — | Two threads touch same memory, one writes, without sync → UB. Lab 04 teaches avoidance. |
| **Real-time safe** | — | Code safe to run in audio callback: bounded time, no blocking, no allocation. |
| **Producer / consumer** | — | UI thread produces commands; audio thread consumes them each buffer. |

---

## Message examples (Lab 04+)

Python will send comma-separated control strings over ZMQ, e.g.:

- `NOTE_ON,440.0` — start a note at 440 Hz  
- `NOTE_OFF,0` — release the note  
- `CUTOFF,1200.0` — set filter cutoff in Hz (Lab 05)  

The C++ side pushes these into a **LockFreeQueue**; the callback **drains** the queue once per buffer (not once per sample).

---

## Math cheat sheet (Lab 01)

- One cycle = **2π radians**  
- Phase step per sample:  
  `phaseIncrement = 2π × frequency / sampleRate`  
- Wrap phase: if `phase >= 2π`, subtract `2π` (or use `fmod`)  
- Sample: `sin(phase)` for a sine wave  

You will implement exactly those three ideas in Lab 01.
