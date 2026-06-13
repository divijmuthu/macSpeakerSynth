# Lab 04 — Python keyboard, ZeroMQ & explicit multithreading

**Unlock after:** Lab 03 — tests green + you heard `./build/race_synth`.

**Goal:** Press a key in Python → C++ changes pitch in real time — **safely across threads**.

---

## What this lab contributes

Lab 03 gave you an **audio thread** (implicit via miniaudio). Lab 04 adds an explicit **`std::thread`** for control I/O and teaches **why** you cannot pass messages with a mutex in the callback.

```text
  Python process          C++ process
  ─────────────          ─────────────────────────────────────
  py_interface/main.py   main thread: start audio, join on exit
       │ ZMQ PUB         ZMQ thread (std::thread): recv → push queue
       └────────────────► audio thread: drain queue → Voice
```

| You build | C++ keywords / patterns |
|-----------|-------------------------|
| `LockFreeQueue.h` | SPSC ring buffer, atomics, memory ordering |
| ZMQ subscriber thread | `std::thread`, join on shutdown, `std::atomic<bool>` run flag |
| `drainControlQueue()` | Consumer on audio thread only — no locks |

---

## Files (when published)

- `include/LockFreeQueue.h` — fill-in-the-blank SPSC queue
- `src/main.cpp` — spawn ZMQ thread, graceful shutdown
- `src/AudioCore.cpp` — call `drainControlQueue()` at start of `renderBlock`
- `py_interface/main.py` — keyboard → `["NOTE_ON", freq]`

See [ROADMAP.md](../ROADMAP.md) · [GLOSSARY.md](../GLOSSARY.md) → SPSC, lock-free, data race.
