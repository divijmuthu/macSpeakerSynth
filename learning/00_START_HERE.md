# RACE Synth — Learn-by-Building Guide

Welcome. Each lab adds real code to the synth. Read the **roadmap** for the full arc (DSP, threading, filters, effects).

**Roadmap:** [ROADMAP.md](ROADMAP.md) — curriculum map, threading arc, all 8 labs

**Jargon:** [GLOSSARY.md](GLOSSARY.md)

---

## How a lab works

1. **Read [ROADMAP.md](ROADMAP.md)** — know where today's code lands.
2. **Read the lab README** — concepts, then TODOs.
3. **Edit `src/`** — search for `TODO (Lab`.
4. **Run tests** for that lab.
5. **All `ok` → next lab.**

---

## Lab sequence

| Lab | Topic | DSP | C++ / systems |
|-----|--------|-----|----------------|
| **01** ✓ | Oscillator | Phase accumulator | Classes, tests |
| **02** ✓ | Envelope + Voice | ADSR FSM | `enum class`, state machines |
| **03** | AudioCore | Buffer/block I/O | Callbacks, 2-thread model, real-time rules |
| **04** | ZMQ + queue | Control vs audio rate | `std::thread`, lock-free, atomics |
| **05** | Biquad LPF | IIR difference eq | Coefficient math |
| **06** | SVF filter | Multimode filter | Better modulated cutoff |
| **07** | Delay + saturation | Time + nonlinear FX | Ring buffer, wet/dry |
| **08** | Polyphony | Mixing, voice steal | Fixed voice array |

---

## Build & test

```bash
cmake -S . -B build && cmake --build build
./build/test_lab03    # current
./build/race_synth    # Lab 03+ — speakers
```

---

## Current step

**Lab 03** → [lab03/README.md](lab03/README.md)
