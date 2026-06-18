# Lab 04 — Python keyboard, ZeroMQ & explicit multithreading

**Goal:** Press a key in Python → C++ changes pitch in real time — **safely across three threads**.

**Prerequisites:** Lab 03 done (`./build/test_lab03`, `./build/race_synth` heard).

**Read:** [ROADMAP.md](../ROADMAP.md) threading arc · [GLOSSARY.md](../GLOSSARY.md) → SPSC, lock-free.

---

## What Lab 04 contributes

Lab 03 moved samples to speakers on miniaudio’s **audio thread**. Lab 04 adds **control** from another process without data races:

```text
  Python process                    C++ process
  ──────────────                    ─────────────────────────────────────────
  py_interface/main.py              MAIN THREAD
  terminal keyboard                      │ start audio, sleep until quit
       │                                 │
       │ ZMQ PUB ──connect──►             │
       │                    tcp://127.0.0.1:5555
       │                                 │
       │                          ZMQ THREAD (std::thread)  ◄── Lab 04 Q5
       │                                 │ zmq_recv → parse → queue.push
       │                                 ▼
       │                          LockFreeQueue (SPSC)
       │                                 │ queue.pop
       │                                 ▼
       │                          AUDIO THREAD (miniaudio)
       │                                 drainControlQueue → Voice
       └────────────────────────────────► speakers
```

| Component | Thread | Role in finished synth |
|-----------|--------|--------------------------|
| `LockFreeQueue` | Between ZMQ + audio | All UI/MIDI messages cross this bridge |
| `parseControlMessage` | ZMQ | Split `STATE,freq` wire string into `ControlMessage` |
| `drainControlQueue` | Audio | Apply control at buffer rate (not per sample) |
| `zmqSubscriberThread` | ZMQ | Never touch `Voice` directly — only push to queue |
| `py_interface/main.py` | Python | Keyboard UI (Lab 05+ adds sliders) |

**Why not a `mutex`?** If the audio thread blocked waiting for Python, you would miss the next buffer deadline → crackle. Lock-free SPSC: producer never waits on consumer, consumer never locks.

---

## Thread count after Lab 04

| # | Thread | You create it? |
|---|--------|----------------|
| 1 | Main | yes (`main()`) |
| 2 | Audio | miniaudio (Lab 03) |
| 3 | ZMQ subscriber | yes — `std::thread` (Lab 04 Q5) |
| — | Python | separate process, talks via TCP |

---

## Files you edit

| File | Questions |
|------|-----------|
| `include/LockFreeQueue.h` | Q1 `push`, Q2 `pop` |
| `src/ControlParser.cpp` | Q3 parse frequency from JSON |
| `src/AudioCore.cpp` | Q4 `drainControlQueue` |
| `src/main.cpp` | Q5 ZMQ subscriber loop |

**Provided (read, don’t rewrite):** `ControlMessage.h`, `ControlParser.h`, `py_interface/main.py`

---

## Dependencies

```bash
brew install zeromq
pip install -r py_interface/requirements.txt
```

---

## Build & run

```bash
cmake -S . -B build
cmake --build build
./build/test_lab04
```

Terminal 1:

```bash
./build/race_synth
```

Terminal 2:

```bash
python py_interface/main.py
```

Type `h` + Enter → 440 Hz (A4). Type `k` → C5. Type `r` → release. `q` → quit Python.

---

## Concept check

1. Why drain the queue **once per buffer**, not once per sample?  
   *(Control rate ≪ audio rate; NOTE_ON is a discrete event, not 48k/sec data.)*

2. Who may call `push`? Who may call `pop`?  
   *(ZMQ thread push only; audio thread pop only — SPSC.)*

3. Why does C++ **bind** and Python **connect**?  
   *(Someone must bind the TCP port; engine is the stable server.)*

4. What happens if the queue is full when you push?  
   *(Message dropped — OK for lab; production might log or use larger queue.)*

---

## Question 1 — `LockFreeQueue::push` (producer)

**TODO:** `Lab 04, Q1` in `include/LockFreeQueue.h`

SPSC ring buffer:

```text
  write_idx_ ──► next empty slot to write
  read_idx_  ──► next slot to read

  empty: read_idx == write_idx
  full:  (write_idx + 1) % Capacity == read_idx
```

Use `memory_order_acquire` when reading the *other* index, `memory_order_release` when publishing your index update.

---

## Question 2 — `LockFreeQueue::pop` (consumer)

**TODO:** `Lab 04, Q2`

Mirror of Q1 on the read side. Only the audio thread calls this.

---

## Wire protocol (Python ↔ C++)

Agreed format — **not JSON**:

```text
NOTE_ON,<frequency>    e.g. NOTE_ON,440.000000
NOTE_OFF,<ignored>      e.g. NOTE_OFF,0
```

Python sends with an f-string; C++ splits on the **first comma**:

1. Before comma → state token (`NOTE_ON` / `NOTE_OFF`)
2. After comma → frequency (`std::from_chars` on NOTE_ON; ignored on NOTE_OFF)

Future labs add fields the same way, e.g. `CUTOFF,1200.0` in Lab 05.

---

## Question 3 — parse frequency (second field)

**File:** `src/ControlParser.cpp` — **already implemented** using the wire format above.

If you want practice: read the implementation, then trace a `NOTE_ON,880.0` message from Python through `zmq_recv` → `parseControlMessage` → `drainControlQueue`.

---

## Question 4 — `drainControlQueue`

**TODO:** `Lab 04, Q4` in `src/AudioCore.cpp`

Already called at the top of `renderBlock`. Drain **all** pending messages in a `while (pop)` loop so a burst of Python keys doesn’t wait multiple buffers.

```cpp
case ControlType::NoteOn:
    voice_.setFrequency(message.frequencyHz, kSampleRate);
    voice_.noteOn();
    break;
case ControlType::NoteOff:
    voice_.noteOff();
    break;
```

---

## Question 5 — ZMQ subscriber thread

**TODO:** `Lab 04, Q5` in `src/main.cpp`

Skeleton:

```cpp
void* ctx = zmq_ctx_new();
void* sub = zmq_socket(ctx, ZMQ_SUB);
zmq_bind(sub, kZmqEndpoint);
zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);

while (g_running.load(std::memory_order_acquire)) {
    char buf[256];
    const int n = zmq_recv(sub, buf, sizeof(buf) - 1, 0);
    if (n <= 0) continue;
    buf[n] = '\0';
    ControlMessage msg;
    if (parseControlMessage(buf, msg)) {
        g_controlQueue.push(msg);
    }
}
zmq_close(sub);
zmq_ctx_destroy(ctx);
```

Use non-blocking recv optional stretch: `ZMQ_RCVTIMEO` so the loop can check `g_running` periodically.

---

## What the tests check

| Test | Validates |
|------|-----------|
| `queue push/pop` | Q1, Q2 basic path |
| `queue empty pop` | Q2 empty case |
| `parse NOTE_OFF` | parser |
| `parse NOTE_ON frequency` | Q3 |
| `drain NOTE_ON` | Q4 → audible output |
| `drain frequency change` | Q4 → different pitch |
| `SPSC producer/consumer` | Q1+Q2 under two `std::thread`s |

---

## When you are done

1. `./build/test_lab04` — all `ok`
2. `./build/race_synth` + Python — keys change pitch live
3. Check off Lab 04 on [ROADMAP.md](../ROADMAP.md)

Next: [lab05/README.md](../lab05/README.md) — Biquad filter.

---

## Optional stretch

- SIGINT handler sets `g_running = false` for clean Ctrl+C shutdown
- `ZMQ_RCVTIMEO` so ZMQ thread wakes to check `g_running`
- Log dropped messages when `push` returns false
