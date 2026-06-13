# Lab 06 — State-variable filter (better filter)

**Unlock after:** Lab 05.

**Goal:** Replace or augment the biquad with an **SVF** (state-variable filter) for smoother cutoff sweeps and multiple modes (LP / HP / BP).

---

## Why not stop at biquad?

Biquads work, but **modulating cutoff** every sample can cause zipper noise and numerical quirks at high cutoffs. SVF (especially trapezoidal / TPT form) is the modern default in many synths for **real-time sweeps**.

| vs Lab 05 biquad | SVF advantage |
|------------------|---------------|
| One output mode | LP, HP, BP from same state |
| Direct form can get noisy when coeffs change fast | TPT structure more stable under modulation |

**Files:** `StateVariableFilter.h/.cpp`, mode enum, wire into `Voice`

See [ROADMAP.md](../ROADMAP.md) · [GLOSSARY.md](../GLOSSARY.md) → SVF, TPT.
