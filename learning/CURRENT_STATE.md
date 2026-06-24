# Current State Snapshot (June 2026)

This is the "what is true right now?" reference for the project.

Related docs:

- `learning/INTERVIEW_READY_RUNDOWN.md`
- `learning/PERFORMANCE_TRACKING.md`
- `learning/REFERENCE_IMPLEMENTATIONS.md`

## Completed labs

- Lab 01-08 complete (oscillator, ADSR, callback audio, ZMQ threading, filters, delay/drive, polyphony).
- Lab 09 started/active (waveform exploration).
- Lab 10-11 available as extension tracks (NEON benchmark, Core Audio backend).

## Current signal path

`Voice[n] (Oscillator -> Filter -> Envelope gain) -> poly mix -> master gain -> delay/wet -> soft clip -> DAC`

## Runtime controls in UI

- Keyboard note on/off (mouse + computer keys).
- Waveform (`WAVEFORM`).
- Filter cutoff + mode.
- Effects: delay time, feedback, wet, drive.
- Master gain (`GAIN`).
- Scope showing output + envelope trend.

## Defaults at UI start

- `WAVEFORM=0` (sine)
- `DELAY=0.25`
- `FEEDBACK=0`
- `WET=0`
- `DRIVE=1.0` (effectively bypass)
- `GAIN=0.8`

## Known tonal caveats

- Saw/square chords can sound buzzy/harsh at higher notes due to naive waveform aliasing.
- Polyphonic loudness and transient behavior are sensitive to mix/headroom strategy.
- This project is a synth engine, not a physical piano model.

## Lab 11 backend override

On macOS builds with Core Audio enabled, backend can be selected at runtime:

- `RACE_AUDIO_BACKEND=coreaudio ./build/race_synth`
- `RACE_AUDIO_BACKEND=miniaudio ./build/race_synth`

## If behavior seems different than expected

1. Rebuild and relaunch both synth and UI.
2. Verify defaults are actually sent (watch status line after startup interactions).
3. Test with sine + effects off first before judging chord stability.
4. Use `learning/REFERENCE_IMPLEMENTATIONS.md` comparison checklist.
