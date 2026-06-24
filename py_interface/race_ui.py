"""Tkinter piano + controls for the RACE synth."""

from __future__ import annotations

import tkinter as tk
from tkinter import ttk

from zmq_client import MODE_NAMES, ScopeClient, SynthClient, SCOPE_SAMPLES, WAVEFORM_NAMES

BASE_FREQ = 261.63

# Physical key → piano label (C4–B5). Hold multiple keys for chords.
COMPUTER_KEY_TO_LABEL: dict[str, str] = {
    "a": "C",
    "w": "C#",
    "s": "D",
    "e": "D#",
    "d": "E",
    "f": "F",
    "t": "F#",
    "g": "G",
    "y": "G#",
    "h": "A",
    "u": "A#",
    "j": "B",
    "k": "C5",
    "o": "C#5",
    "l": "D5",
    "p": "D#5",
    "semicolon": "E5",
    "quote": "F5",
    "bracketright": "F#5",
    "backslash": "G5",
    "z": "G#5",
    "x": "A5",
    "c": "A#5",
    "v": "B5",
}

LABEL_TO_COMPUTER_KEY = {label: key for key, label in COMPUTER_KEY_TO_LABEL.items()}


def freq_for_semitone(semitone: int) -> float:
    return BASE_FREQ * (2.0 ** (semitone / 12.0))


KEY_LAYOUT: list[tuple[str, int, bool, int | None]] = [
    ("C", 0, False, None),
    ("C#", 1, True, 0),
    ("D", 2, False, None),
    ("D#", 3, True, 1),
    ("E", 4, False, None),
    ("F", 5, False, None),
    ("F#", 6, True, 3),
    ("G", 7, False, None),
    ("G#", 8, True, 4),
    ("A", 9, False, None),
    ("A#", 10, True, 5),
    ("B", 11, False, None),
    ("C5", 12, False, None),
    ("C#5", 13, True, 7),
    ("D5", 14, False, None),
    ("D#5", 15, True, 8),
    ("E5", 16, False, None),
    ("F5", 17, False, None),
    ("F#5", 18, True, 10),
    ("G5", 19, False, None),
    ("G#5", 20, True, 11),
    ("A5", 21, False, None),
    ("A#5", 22, True, 12),
    ("B5", 23, False, None),
]

LABEL_TO_SEMITONE = {label: semi for label, semi, _, _ in KEY_LAYOUT}


class RaceSynthUI(tk.Tk):
    WHITE_W = 44
    WHITE_H = 130
    BLACK_W = 26
    BLACK_H = 78
    SCOPE_H = 200

    def __init__(self, client: SynthClient, scope: ScopeClient | None = None) -> None:
        super().__init__()
        self.client = client
        self.scope_client = scope
        self._active_notes: set[str] = set()
        self._note_freqs: dict[str, float] = {}
        self._physical_keys_held: set[str] = set()
        self._pending_key_off_jobs: dict[str, str] = {}
        self._filter_mode = tk.IntVar(value=0)
        self._waveform = tk.IntVar(value=0)
        self._simd_mode = tk.IntVar(value=0)
        self._scope_job: str | None = None
        self._white_rects: dict[str, int] = {}
        self._black_rects: dict[str, int] = {}
        self._key_active_fill: dict[str, str] = {}

        self.title("RACE Synth")
        self.minsize(680, 820)
        self.geometry("720x920")
        self.resizable(True, True)
        self.configure(padx=10, pady=10)

        self.columnconfigure(0, weight=1)
        self.rowconfigure(5, weight=1)

        self._build_header()
        self._build_controls_row()
        self._build_effects_panel()
        self._build_keyboard()
        self._build_scope_panel()
        self._build_status()

        self.bind("<KeyPress>", self._on_computer_key_press, add="+")
        self.bind("<KeyRelease>", self._on_computer_key_release, add="+")
        self.bind("<Button-1>", self._on_root_click, add="+")

        self.protocol("WM_DELETE_WINDOW", self._on_close)
        if self.scope_client is not None:
            self._schedule_scope_tick()

    def _build_header(self) -> None:
        ttk.Label(
            self,
            text="RACE — Real-time Audio Control Engine",
            font=("TkDefaultFont", 15, "bold"),
        ).grid(row=0, column=0, sticky="w", pady=(0, 4))

        ttk.Label(
            self,
            text="Chords: hold computer keys  A W S E D F G H J  (C4–B4)  ·  K O L P ; '  (C5–F5)  ·  Z X C V (G5–B5)",
            foreground="#555",
            wraplength=700,
        ).grid(row=1, column=0, sticky="w", pady=(0, 8))

    def _build_controls_row(self) -> None:
        row = ttk.Frame(self)
        row.grid(row=2, column=0, sticky="ew", pady=(0, 8))
        row.columnconfigure(0, weight=1)
        row.columnconfigure(1, weight=1)

        wf_panel = ttk.LabelFrame(row, text="Oscillator (Lab 09)")
        wf_panel.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        ttk.Label(wf_panel, text="Waveform:").pack(anchor="w", padx=8, pady=(8, 2))
        wf_btns = ttk.Frame(wf_panel)
        wf_btns.pack(anchor="w", padx=8, pady=(0, 8))
        for idx, name in enumerate(WAVEFORM_NAMES):
            ttk.Radiobutton(
                wf_btns,
                text=name,
                value=idx,
                variable=self._waveform,
                command=self._apply_waveform,
            ).pack(side="left", padx=3)
        ttk.Checkbutton(
            wf_panel,
            text="SIMD batch (runtime)",
            variable=self._simd_mode,
            command=self._apply_simd_mode,
        ).pack(anchor="w", padx=8, pady=(0, 8))

        filt_panel = ttk.LabelFrame(row, text="Filter (SVF)")
        filt_panel.grid(row=0, column=1, sticky="nsew", padx=(6, 0))

        cutoff_row = ttk.Frame(filt_panel)
        cutoff_row.pack(fill="x", padx=8, pady=8)
        ttk.Label(cutoff_row, text="Cutoff (Hz):").pack(side="left")
        self.cutoff_entry = ttk.Entry(cutoff_row, width=10)
        self.cutoff_entry.insert(0, "1200")
        self.cutoff_entry.pack(side="left", padx=(6, 6))
        ttk.Button(cutoff_row, text="Apply", command=self._apply_cutoff).pack(side="left")

        mode_row = ttk.Frame(filt_panel)
        mode_row.pack(fill="x", padx=8, pady=(0, 8))
        ttk.Label(mode_row, text="Mode:").pack(side="left")
        for idx, name in enumerate(MODE_NAMES):
            ttk.Radiobutton(
                mode_row,
                text=name,
                value=idx,
                variable=self._filter_mode,
                command=self._apply_filter_mode,
            ).pack(side="left", padx=4)

    def _build_effects_panel(self) -> None:
        panel = ttk.LabelFrame(self, text="Effects (Lab 07)")
        panel.grid(row=3, column=0, sticky="ew", pady=(0, 8))

        grid = ttk.Frame(panel)
        grid.pack(fill="x", padx=8, pady=6)
        labels = [
            ("Delay (s)", "0.25"),
            ("Feedback", "0"),
            ("Wet mix", "0"),
            ("Drive", "1"),
            ("Volume", "0.8"),
        ]
        self.effect_entries: dict[str, ttk.Entry] = {}
        for col, (name, default) in enumerate(labels):
            cell = ttk.Frame(grid)
            cell.grid(row=0, column=col, padx=(0, 12), sticky="w")
            ttk.Label(cell, text=f"{name}:").pack(anchor="w")
            entry = ttk.Entry(cell, width=8)
            entry.insert(0, default)
            entry.pack(anchor="w")
            self.effect_entries[name] = entry
        ttk.Button(panel, text="Apply effects", command=self._apply_effects).pack(
            anchor="w", padx=8, pady=(0, 8)
        )

    def _build_keyboard(self) -> None:
        frame = ttk.LabelFrame(
            self, text="On-screen piano (optional) — use computer keyboard for chords"
        )
        frame.grid(row=4, column=0, sticky="ew", pady=(0, 8))

        white_count = sum(1 for _, _, is_black, _ in KEY_LAYOUT if not is_black)
        canvas_w = white_count * self.WHITE_W + 4

        self.piano = tk.Canvas(
            frame,
            width=canvas_w,
            height=self.WHITE_H + 8,
            bg="#2b2b2b",
            highlightthickness=0,
        )
        self.piano.pack(padx=8, pady=8)

        white_idx = 0
        for label, semi, is_black, _ in KEY_LAYOUT:
            if is_black:
                continue
            x0 = 2 + white_idx * self.WHITE_W
            x1 = x0 + self.WHITE_W - 2
            rid = self.piano.create_rectangle(
                x0, 4, x1, self.WHITE_H, fill="#fafafa", outline="#888", width=1
            )
            pc_key = LABEL_TO_COMPUTER_KEY.get(label, "")
            key_hint = pc_key.upper() if len(pc_key) == 1 else pc_key[:3].upper()
            self.piano.create_text(
                (x0 + x1) / 2,
                self.WHITE_H - 28,
                text=label,
                fill="#333",
                font=("TkDefaultFont", 9, "bold"),
            )
            if key_hint:
                self.piano.create_text(
                    (x0 + x1) / 2,
                    self.WHITE_H - 12,
                    text=key_hint,
                    fill="#888",
                    font=("TkDefaultFont", 8),
                )
            self._bind_key_mouse(label, rid)
            self._white_rects[label] = rid
            self._key_active_fill[label] = "#dcdcdc"
            white_idx += 1

        for label, semi, is_black, anchor_white in KEY_LAYOUT:
            if not is_black or anchor_white is None:
                continue
            x_center = 2 + anchor_white * self.WHITE_W + self.WHITE_W
            x0 = x_center - self.BLACK_W // 2
            x1 = x0 + self.BLACK_W
            rid = self.piano.create_rectangle(
                x0, 4, x1, self.BLACK_H, fill="#1a1a1a", outline="#000", width=1
            )
            self._bind_key_mouse(label, rid)
            self._black_rects[label] = rid
            self._key_active_fill[label] = "#444"

    def _bind_key_mouse(self, label: str, rect_id: int) -> None:
        self.piano.tag_bind(
            rect_id, "<ButtonPress-1>", lambda _e, lbl=label: self._note_on(lbl)
        )
        self.piano.tag_bind(
            rect_id, "<ButtonRelease-1>", lambda _e, lbl=label: self._note_off(lbl)
        )

    def _is_text_entry(self, widget: tk.Misc | None) -> bool:
        w: tk.Misc | None = widget
        while w is not None and w is not self:
            if isinstance(w, (tk.Entry, ttk.Entry)):
                return True
            if w.winfo_class() in ("Entry", "TEntry"):
                return True
            w = w.master  # type: ignore[assignment]
        return False

    def _typing_in_entry(self) -> bool:
        return self._is_text_entry(self.focus_get())

    def _on_root_click(self, event: tk.Event) -> None:
        # Don't steal focus from cutoff / effects text fields.
        if self._is_text_entry(event.widget):
            return
        self.focus_set()

    def _on_computer_key_press(self, event: tk.Event) -> None:
        if self._typing_in_entry():
            return
        keysym = event.keysym.lower()
        pending_job = self._pending_key_off_jobs.pop(keysym, None)
        if pending_job is not None:
            self.after_cancel(pending_job)
        if keysym in self._physical_keys_held:
            return
        label = COMPUTER_KEY_TO_LABEL.get(keysym)
        if label is None:
            return
        self._physical_keys_held.add(keysym)
        self._note_on(label)

    def _on_computer_key_release(self, event: tk.Event) -> None:
        if self._typing_in_entry():
            return
        keysym = event.keysym.lower()
        if keysym not in self._physical_keys_held:
            return
        label = COMPUTER_KEY_TO_LABEL.get(keysym)
        if label is None:
            return
        if keysym in self._pending_key_off_jobs:
            return
        # Delay NOTE_OFF slightly to absorb OS key-repeat press/release jitter.
        self._pending_key_off_jobs[keysym] = self.after(
            25, lambda ks=keysym, lbl=label: self._finalize_computer_key_release(ks, lbl)
        )

    def _finalize_computer_key_release(self, keysym: str, label: str) -> None:
        self._pending_key_off_jobs.pop(keysym, None)
        self._physical_keys_held.discard(keysym)
        self._note_off(label)

    def _note_on(self, label: str) -> None:
        semi = LABEL_TO_SEMITONE.get(label)
        if semi is None:
            return
        freq = freq_for_semitone(semi)

        if label in self._active_notes:
            self.client.note_on(freq)
            self._refresh_playing_status()
            return

        self._active_notes.add(label)
        self._note_freqs[label] = freq
        self.client.note_on(freq)
        self._highlight_key(label, True)
        self._refresh_playing_status()

    def _note_off(self, label: str) -> None:
        if label not in self._active_notes:
            return
        self._active_notes.discard(label)
        freq_off = self._note_freqs.pop(label, freq_for_semitone(LABEL_TO_SEMITONE[label]))
        self.client.note_off(freq_off)
        self._highlight_key(label, False)
        self._refresh_playing_status()

    def _refresh_playing_status(self) -> None:
        if not self._active_notes:
            self.status.set("Ready — click here, then play (A S D F = C major)")
            return
        parts = [
            f"{lbl} {self._note_freqs[lbl]:.1f} Hz"
            for lbl in sorted(self._active_notes, key=lambda x: self._note_freqs[x])
        ]
        self.status.set(f"Playing: {', '.join(parts)}")

    def _highlight_key(self, label: str, active: bool) -> None:
        rid = self._black_rects.get(label) or self._white_rects.get(label)
        if rid is None:
            return
        if active:
            self.piano.itemconfig(rid, fill=self._key_active_fill[label])
        else:
            base = "#1a1a1a" if label in self._black_rects else "#fafafa"
            self.piano.itemconfig(rid, fill=base)

    def _build_scope_panel(self) -> None:
        white_count = sum(1 for _, _, is_black, _ in KEY_LAYOUT if not is_black)
        self.scope_w = max(white_count * self.WHITE_W + 4, 640)

        frame = ttk.LabelFrame(self, text="Oscilloscope — last 256 samples")
        frame.grid(row=5, column=0, sticky="nsew", pady=(0, 8))
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)

        self.scope_canvas = tk.Canvas(
            frame,
            width=self.scope_w,
            height=self.SCOPE_H,
            bg="#080810",
            highlightthickness=0,
        )
        self.scope_canvas.grid(row=0, column=0, sticky="nsew", padx=8, pady=8)

        legend = ttk.Frame(frame)
        legend.grid(row=1, column=0, sticky="w", padx=8, pady=(0, 6))
        ttk.Label(legend, text="● output", foreground="#4ee8d8").pack(side="left", padx=(0, 12))
        ttk.Label(legend, text="● envelope (ADSR)", foreground="#e8a045").pack(side="left")

        self._scope_wave_h = int(self.SCOPE_H * 0.58)
        self._scope_env_top = self._scope_wave_h + 4
        self._draw_scope_grid()

    def _draw_scope_grid(self) -> None:
        w, h = self.scope_w, self.SCOPE_H
        wave_h = self._scope_wave_h
        mid = wave_h // 2

        self.scope_canvas.delete("grid")
        for x in range(0, w, 40):
            self.scope_canvas.create_line(x, 0, x, h, fill="#141422", tags="grid")
        self.scope_canvas.create_line(0, mid, w, mid, fill="#2a2a44", tags="grid")
        self.scope_canvas.create_line(0, self._scope_env_top, w, self._scope_env_top, fill="#2a2a44", tags="grid")

    def _schedule_scope_tick(self) -> None:
        self._scope_job = self.after(33, self._tick_scope)

    def _tick_scope(self) -> None:
        if self.scope_client is None:
            return
        data = self.scope_client.poll()
        if data is not None:
            self._draw_scope_traces(*data)
        self._schedule_scope_tick()

    def _draw_scope_traces(self, output, envelope) -> None:
        w = max(self.scope_canvas.winfo_width(), self.scope_w)
        h = max(self.scope_canvas.winfo_height(), self.SCOPE_H)
        wave_h = int(h * 0.58)
        env_top = wave_h + 4
        env_h = max(h - env_top - 4, 1)
        mid = wave_h // 2
        n = min(len(output), SCOPE_SAMPLES)

        self.scope_canvas.delete("trace")

        env_pts: list[float] = [0, h - 4]
        for i in range(n):
            x = i * w / SCOPE_SAMPLES
            y = env_top + (1.0 - max(0.0, min(1.0, envelope[i]))) * env_h
            env_pts.extend([x, y])
        env_pts.extend([w, h - 4, 0, h - 4])
        self.scope_canvas.create_polygon(*env_pts, fill="#3a2810", outline="", tags="trace")

        env_line: list[float] = []
        for i in range(n):
            x = i * w / SCOPE_SAMPLES
            y = env_top + (1.0 - max(0.0, min(1.0, envelope[i]))) * env_h
            env_line.extend([x, y])
        if len(env_line) >= 4:
            self.scope_canvas.create_line(*env_line, fill="#e8a045", width=1.5, tags="trace")

        wave_pts: list[float] = []
        for i in range(n):
            x = i * w / SCOPE_SAMPLES
            y = mid - max(-1.0, min(1.0, output[i])) * (wave_h * 0.42)
            wave_pts.extend([x, y])
        if len(wave_pts) >= 4:
            self.scope_canvas.create_line(*wave_pts, fill="#1a5a55", width=3.0, tags="trace")
            self.scope_canvas.create_line(*wave_pts, fill="#4ee8d8", width=1.2, smooth=True, tags="trace")

        self.scope_canvas.tag_raise("trace")
        self.scope_canvas.tag_lower("grid")

    def _build_status(self) -> None:
        self.status = tk.StringVar(value="Click here, then play: A S D F = C major chord")
        ttk.Label(self, textvariable=self.status, foreground="#555").grid(
            row=6, column=0, sticky="w"
        )

    def _apply_waveform(self) -> None:
        wf = self._waveform.get()
        self.client.waveform(wf)
        self.status.set(f"WAVEFORM {wf} ({WAVEFORM_NAMES[wf]})")

    def _apply_simd_mode(self) -> None:
        enabled = self._simd_mode.get() == 1
        self.client.simd_mode(enabled)
        self.status.set(f"SIMD runtime mode: {'ON' if enabled else 'OFF'}")

    def _apply_cutoff(self) -> None:
        try:
            hz = float(self.cutoff_entry.get())
        except ValueError:
            self.status.set("Invalid cutoff — enter a number in Hz")
            return
        self.client.cutoff(hz)
        self.status.set(f"CUTOFF {hz:.0f} Hz")

    def _apply_filter_mode(self) -> None:
        mode = self._filter_mode.get()
        self.client.filter_mode(mode)
        self.status.set(f"MODE {mode} ({MODE_NAMES[mode]})")

    def _apply_effects(self) -> None:
        try:
            delay_s = float(self.effect_entries["Delay (s)"].get())
            feedback = float(self.effect_entries["Feedback"].get())
            wet = float(self.effect_entries["Wet mix"].get())
            drive = float(self.effect_entries["Drive"].get())
            volume = float(self.effect_entries["Volume"].get())
        except ValueError:
            self.status.set("Invalid effect value — enter numbers")
            return
        self.client.delay_time(delay_s)
        self.client.delay_feedback(feedback)
        self.client.delay_wet(wet)
        self.client.drive(drive)
        self.client.master_gain(volume)
        self.status.set(
            "FX "
            f"delay={delay_s:.2f}s fb={feedback:.2f} wet={wet:.2f} "
            f"drive={drive:.2f} vol={volume:.2f}"
        )

    def _on_close(self) -> None:
        if self._scope_job is not None:
            self.after_cancel(self._scope_job)
            self._scope_job = None
        for job in self._pending_key_off_jobs.values():
            self.after_cancel(job)
        self._pending_key_off_jobs.clear()
        try:
            self.client.note_off(0.0)
        except Exception:
            pass
        self.client.close()
        if self.scope_client is not None:
            self.scope_client.close()
        self.destroy()


def run_ui() -> None:
    client = SynthClient()
    scope = ScopeClient()
    # Dry defaults — bypass delay + unity drive until user clicks Apply effects.
    client.waveform(0)  # Start from sine for clean baseline.
    client.delay_time(0.25)
    client.delay_feedback(0.0)
    client.delay_wet(0.0)
    client.drive(1.0)
    client.master_gain(0.8)
    client.simd_mode(False)
    app = RaceSynthUI(client, scope)
    app.mainloop()
