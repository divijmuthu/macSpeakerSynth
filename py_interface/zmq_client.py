"""ZeroMQ wire helpers for the RACE Python controller."""

from __future__ import annotations

import struct
import time
from typing import Sequence

import zmq

ENDPOINT = "tcp://127.0.0.1:5555"
SCOPE_ENDPOINT = "tcp://127.0.0.1:5556"
SCOPE_SAMPLES = 256
SCOPE_MAGIC = b"SCOP"
SCOPE_PAYLOAD = 4 + SCOPE_SAMPLES * 4 * 2

MODE_NAMES = ("LP", "HP", "BP")
WAVEFORM_NAMES = ("Sine", "Saw", "Square", "Triangle")


class SynthClient:
    def __init__(self) -> None:
        self._ctx = zmq.Context()
        self._pub = self._ctx.socket(zmq.PUB)
        self._pub.connect(ENDPOINT)
        time.sleep(0.3)  # PUB/SUB slow-joiner

    def close(self) -> None:
        self._pub.close()
        self._ctx.term()

    def note_on(self, freq: float) -> None:
        self._pub.send_string(f"NOTE_ON,{freq:.6f}")

    def note_off(self, freq: float = 0.0) -> None:
        self._pub.send_string(f"NOTE_OFF,{freq:.6f}")

    def cutoff(self, hz: float) -> None:
        self._pub.send_string(f"CUTOFF,{hz:.6f}")

    def filter_mode(self, mode: int) -> None:
        self._pub.send_string(f"MODE,{mode}")

    def delay_time(self, seconds: float) -> None:
        self._pub.send_string(f"DELAY,{seconds:.6f}")

    def delay_feedback(self, amount: float) -> None:
        self._pub.send_string(f"FEEDBACK,{amount:.6f}")

    def delay_wet(self, mix: float) -> None:
        self._pub.send_string(f"WET,{mix:.6f}")

    def drive(self, amount: float) -> None:
        self._pub.send_string(f"DRIVE,{amount:.6f}")

    def waveform(self, index: int) -> None:
        self._pub.send_string(f"WAVEFORM,{index}")


class ScopeClient:
    """SUB socket for mono output + envelope snapshots from race_synth."""

    def __init__(self) -> None:
        self._ctx = zmq.Context()
        self._sub = self._ctx.socket(zmq.SUB)
        self._sub.connect(SCOPE_ENDPOINT)
        self._sub.setsockopt(zmq.RCVTIMEO, 10)
        self._sub.setsockopt(zmq.CONFLATE, 1)
        self._sub.subscribe(b"")

    def close(self) -> None:
        self._sub.close()
        self._ctx.term()

    def poll(self) -> tuple[Sequence[float], Sequence[float]] | None:
        try:
            msg = self._sub.recv()
        except zmq.Again:
            return None
        if len(msg) < SCOPE_PAYLOAD or msg[:4] != SCOPE_MAGIC:
            return None
        offset = 4
        output = struct.unpack(f"{SCOPE_SAMPLES}f", msg[offset : offset + SCOPE_SAMPLES * 4])
        offset += SCOPE_SAMPLES * 4
        envelope = struct.unpack(f"{SCOPE_SAMPLES}f", msg[offset : offset + SCOPE_SAMPLES * 4])
        return output, envelope
