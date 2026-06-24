# Reference Implementations and Comparison Checklist

This file helps you sanity-check RACE against real synth projects and common polyphony practices.

## Good open references

- [Surge XT source](https://github.com/surge-synthesizer/surge) - full open-source C/C++ synth with mature voice handling.
- [Surge XT architecture notes](https://github.com/surge-synthesizer/surge/blob/main/doc/Surge%20Architecture.md) - practical details on voice allocation and engine structure.
- [Dexed source](https://github.com/asb2m10/dexed) - FM synth with production voice logic and click-avoidance fixes.
- [JUCE Synthesiser class](https://docs.juce.com/develop/classjuce_1_1Synthesiser.html) - standard reference API for polyphonic voice allocation/stealing behavior.
- [VCV Fundamental VCO source](https://github.com/VCVRack/Fundamental/blob/master/src/VCO.cpp) - useful for oscillator/aliasing tradeoffs and anti-aliasing patterns.

## What to compare against (quick rubric)

Use this when listening or debugging:

1. **Single note baseline**
   - Sine wave, effects off, medium cutoff.
   - Should be stable and click-free on hold/release.
2. **Two- and three-note chord behavior**
   - Check for harshness from mix overage (flat plateaus on scope).
   - Check for pop when adding/removing notes (gain step).
3. **Voice allocation behavior**
   - Re-trigger same note: should not create extra ghost voices.
   - Note-off by pitch should release only matching voice.
   - Voice steal should not hard-click when all voices are busy.
4. **Waveform expectations**
   - Sine/triangle should be smoother.
   - Saw/square are intentionally brighter/rougher and may alias at higher pitches.
5. **Headroom and loudness strategy**
   - Polyphonic sum should preserve dynamics but avoid hard clipping.
   - Master gain should be adjustable from UI.

## "Does our current polyphony make sense?"

Given this project's educational goals, yes, the current architecture is reasonable:

- Fixed `Voice` pool in `AudioCore`.
- Control-thread messages into lock-free queue.
- Allocation/release and mixing fully on audio thread.
- Master-bus effects after voice summing.

The biggest remaining quality gap versus mature synths is not architecture, but polish:

- better anti-aliasing for discontinuous waves,
- more musical gain management/limiting,
- smoother voice-steal fades.

## Suggested A/B listening routine

1. In RACE, set `WAVEFORM=Sine`, effects off, master gain moderate.
2. Play intervals and triads; verify no plateau clipping on scope.
3. Switch to saw/square and note expected extra brightness.
4. Compare against any software synth default patch (e.g. Surge XT init patch):
   - you should match note stability and chord consistency,
   - but tone color will differ (that's expected without piano modeling).
