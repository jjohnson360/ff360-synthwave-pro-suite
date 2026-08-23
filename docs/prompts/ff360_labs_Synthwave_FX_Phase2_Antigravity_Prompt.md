# ff360_labs Synthwave FX Suite — Phase 2: Creative FX
### Antigravity Build Prompt

**Project:** ff360_labs Synthwave Production Collection
**Phase:** 2 of 3 — Creative FX (Neon Width, Neon Tape Stop, Cyberpunk Glitch)
**Prerequisite:** Phase 1 complete — `ff360_dsp_core` exists and compiles, VHS/Neon Chorus/Midnight Reverb are shipped, and the shared visual system (palette, glassmorphic panels, hero-knob style) is in place.
**Goal of this phase:** add two new DSP modules to the core (`StereoEngine`, `GlitchEngine`), reuse `TapeEngine` for the tape-stop behaviors, and ship three plugins that extend the family without duplicating anything Phase 1 already built.
**Targets:** VST3 + AU, same shared-core approach as Phase 1.

---

## 0. Ground rules for the agent

- Do not reimplement anything that already exists in `ff360_dsp_core`. Neon Tape Stop in particular should be a thin control layer over `TapeEngine`'s existing pitch-drift/wow primitives, not a new saturation or noise path.
- New modules (`StereoEngine`, `GlitchEngine`) get added to `ff360_dsp_core` following the same pattern as `TapeEngine`/`ModulationEngine`/`ReverbEngine`: standalone header + implementation, own unit tests.
- Apply the Step 5 visual system from Phase 1 from the first UI pass — no unstyled placeholder UI.
- Where a plugin needs a visual readout beyond standard metering (Neon Width's stereo field, in particular), build it as a reusable UI component so Phase 3 or future plugins can adopt it too.
- Carrying over from Phase 1: each plugin gets its own short UI checkpoint against the mockup and a Windows/WASAPI standalone smoke test as part of its own acceptance criteria — don't defer platform or UI checks to a later consolidation pass, per the issues that surfaced that way in the Metering Suite. Any plugin here that touches metering (all three use standard level displays) should also spot-check against the post-Phase-10 `MeteringBridge`, same as Phase 1.

---

## 1. StereoEngine — new shared module, build first

Backs Neon Width and is written generically enough that any future plugin needing stereo manipulation can use it.

**Location:** `ff360_dsp_core/include/ff360/StereoEngine.h` (+ `src/`, `tests/`)

**Responsibilities:**
- Micro-delay and Haas-effect stereo widening
- Stereo detune (independent pitch offset per channel)
- Mid/Side width control
- Frequency-dependent width (crossover-split processing — narrow low end, wide high end)
- Stereo rotation (rotating the L/R image without collapsing it)
- Bass mono (frequency-dependent mono-summing below a cutoff)
- Phase correlation metering output (feed this to the UI's stereo field display, not just a numeric readout)

**Acceptance:** `StereoEngine` unit tests confirm mono-compatibility never breaks below the Bass Mono cutoff, and phase correlation output tracks a known test signal (fully correlated, fully anti-correlated, and decorrelated cases) within tolerance.

---

## 2. Neon Width — build second

**Parameters:** Micro-delay, Haas, Stereo detune, M/S width, Frequency-dependent width, Stereo rotation, Bass mono, Phase correlation (readout).
**Concept:** MONO → WIDE → HUGE → MOVING as the intended progression across the control set — Micro-delay/Haas get you to WIDE, M/S + frequency-dependent width gets you to HUGE, stereo rotation and detune modulation get you to MOVING.

**Build order:**
1. Wire `StereoEngine` into a bare `AudioProcessor`, confirm all parameters process correctly and stay mono-safe when Bass Mono is engaged.
2. Build the visual stereo field display (real-time L/R correlation + width visualization) as a reusable UI component — this is the plugin's signature feature, called out specifically in the concept doc as worth extra attention.
3. UI pass using the Phase 1 visual system, with the stereo field display as the focal element of the layout.

**Acceptance:** The stereo field visualization updates in real time with no dropped frames at typical buffer sizes, and a mono compatibility check (sum to mono, A/B against Bass Mono on/off) shows no phase cancellation issues when Bass Mono is active. Per the Phase 1 pattern: short UI checkpoint against the mockup plus a Windows standalone smoke test before moving to Neon Tape Stop.

---

## 3. Neon Tape Stop — build third

No new DSP module — this plugin is a control-layer wrapper around `TapeEngine`'s existing Pitch Drift and Wow/Flutter primitives, plus a small state machine for trigger behavior. Keep it lightweight; the concept doc calls this "a small plugin, but extremely useful."

**Parameters:** Trigger source (MIDI / manual / BPM sync), Slowdown, Pitch curve, Filter movement, Reverse recovery, and three stop-character presets — Vinyl stop, Tape stop, Digital stop — which are just different `TapeEngine` parameter curves, not separate DSP paths.

**Build order:**
1. Build the trigger state machine (idle → stopping → stopped → recovering) as a small standalone class, independent of `TapeEngine`, so it can be reused for BPM-synced retriggering.
2. Wire the state machine's output into `TapeEngine`'s pitch/wow parameters over time to produce the stop curve.
3. Implement the three stop-character presets as parameter curve sets, not new code paths.
4. Confirm this plugin can also be instantiated as an internal component (not a standalone plugin) inside VHS and RetroFX, per the concept doc's note that Neon Tape Stop "could also become a component inside VHS and RetroFX" — expose it as a reusable class, not just a plugin wrapper, so Phase 3's RetroFX can embed it directly.

**Acceptance:** Trigger-to-silence timing is sample-accurate against the Slowdown parameter, BPM sync locks correctly to host tempo, and the three stop-character presets are audibly distinct from each other. Per the Phase 1 pattern: short UI checkpoint against the mockup plus a Windows standalone smoke test before moving to Cyberpunk Glitch.

---

## 4. GlitchEngine — new shared module, build fourth

Backs Cyberpunk Glitch.

**Location:** `ff360_dsp_core/include/ff360/GlitchEngine.h` (+ `src/`, `tests/`)

**Responsibilities:**
- Stutter / beat repeat (buffer capture and re-trigger at 1/4, 1/8, 1/16, 1/32 divisions)
- Reverse playback of captured buffers
- Buffer freeze
- Pitch shift on captured material
- Bitcrush and gate, reusing `TapeEngine`'s bit-reduction primitive rather than duplicating it
- Filter (a simple resonant filter stage)
- Randomization engine with a probability parameter per trigger, so glitches don't fire on every subdivision

**Acceptance:** Buffer capture/re-trigger is glitch-free at all four timing divisions when synced to host tempo, and the randomization/probability system produces musically useful (not chaotic-every-time) results at low probability settings.

---

## 5. Cyberpunk Glitch — build fifth

**Parameters:** Stutter, Beat repeat, Reverse, Buffer freeze, Pitch shift, Bitcrush, Gate, Filter, Randomization, with 1/4–1/32 timing controls and per-trigger probability.

**Build order:**
1. Wire `GlitchEngine` into a bare `AudioProcessor`, confirm buffer capture/re-trigger behaves correctly at all timing divisions.
2. Build the trigger UI (timing division selector + probability control) reusing interaction patterns from RetroFX's eventual GENERATE button (Phase 3) where sensible, so the two plugins feel related.
3. UI pass using the Phase 1 visual system.

**Acceptance:** Full automation sweep test with no audio glitches outside the intended glitch effect itself, and host tempo changes mid-playback don't desync the buffer timing. Per the Phase 1 pattern: short UI checkpoint against the mockup plus a Windows standalone smoke test.

---

## Phase 2 deliverable summary

| Plugin | VST3 | AU | New/shared core module |
|---|---|---|---|
| Neon Width | ✓ | ✓ | `StereoEngine` (new) |
| Neon Tape Stop | ✓ | ✓ | `TapeEngine` (reused, no new DSP) |
| Cyberpunk Glitch | ✓ | ✓ | `GlitchEngine` (new) |

By the end of Phase 2, `ff360_dsp_core` contains: `TapeEngine`, `ModulationEngine`, `ReverbEngine`, `MacroSystem`, `ParameterManager`, `MeteringBridge`, `StereoEngine`, `GlitchEngine`. Phase 3 (RetroFX, NightDrive) adds one more module — `GenerativeEngine` — and otherwise composes entirely from what already exists.
