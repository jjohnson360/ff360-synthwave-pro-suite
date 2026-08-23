# ff360_labs Synthwave FX Suite — Phase 3: Generative FX
### Antigravity Build Prompt

**Project:** ff360_labs Synthwave Production Collection
**Phase:** 3 of 3 — Generative FX (RetroFX, NightDrive)
**Prerequisite:** Phases 1 and 2 complete. `ff360_dsp_core` contains `TapeEngine`, `ModulationEngine`, `ReverbEngine`, `MacroSystem`, `ParameterManager`, `MeteringBridge`, `StereoEngine`, `GlitchEngine`, and a reusable tape-stop state machine from Neon Tape Stop.
**Goal of this phase:** add one new shared module (`GenerativeEngine`), close out the product line, and confirm the whole eight-plugin collection shares one DSP core end to end.
**Targets:** VST3 + AU, same shared-core approach as Phases 1 and 2.

---

## 0. Ground rules for the agent

- This is the last phase — treat it as a chance to audit reuse across the whole collection, not just build two more plugins. Both RetroFX and NightDrive lean heavily on modules that already exist; confirm nothing here duplicates DSP that's already in `ff360_dsp_core`.
- RetroFX embeds Neon Tape Stop's trigger/state-machine class directly (per the Phase 2 note that it should be reusable, not just plugin-wrapped) rather than reimplementing a tape-sweep behavior.
- NightDrive's Chord/Scale Lock feature is the one place this phase touches Fred's other in-flight project, ChordFlow — treat the integration as optional/soft-fail: NightDrive should work standalone with a fixed internal scale table if ChordFlow isn't present, and use ChordFlow's live chord output when it is.
- Apply the Phase 1 visual system throughout. RetroFX and NightDrive's GENERATE/EVOLVE macro knobs should visually match VHS's DEGRADE knob exactly — same component, same treatment.
- Carrying over from Phases 1–2: each plugin still gets its own UI checkpoint and Windows/WASAPI smoke test before moving to the next, even this late in the collection — the whole point of catching platform and UI issues per-plugin (instead of in one final pass, the way the Metering Suite's Phase 10 consolidation had to) is that it stays true through the last plugin too, not just the first.

---

## 1. GenerativeEngine — new shared module, build first

Backs both RetroFX and NightDrive.

**Location:** `ff360_dsp_core/include/ff360/GenerativeEngine.h` (+ `src/`, `tests/`)

**Responsibilities:**
- A generator registry: pluggable `IGenerator` interface, so RetroFX's transition generators and NightDrive's ambient generators are both concrete implementations of the same interface rather than parallel systems.
- Seeded randomization (deterministic given a seed, for reproducible "GENERATE" results when a musician wants to recall a specific take).
- BPM-sync scheduling shared by both plugins' sync controls.
- A density/evolve-rate parameter pair that controls how often and how much the generator's output changes over time — used directly by NightDrive's Density/Evolve Rate, and adaptable for RetroFX's GENERATE cadence.

**Acceptance:** Given the same seed and parameters, `GenerativeEngine` produces bit-identical output across runs (critical for recall-safe presets), and swapping `IGenerator` implementations doesn't require touching the scheduling or randomization code.

---

## 2. RetroFX — build second

**Concept:** a generative synthwave transition generator — instead of browsing sample libraries, the plugin creates the transition on demand.

**Generators (each an `IGenerator` implementation):** Noise Sweep, Pitch Sweep, Laser, Reverse, Impact, Riser, Downlifter, Digital Sweep, Tape Sweep.

**Build order:**
1. Implement each generator as a small, focused `IGenerator` class. Tape Sweep should call directly into the Neon Tape Stop trigger/state-machine class from Phase 2 rather than reimplementing sweep timing — this is the concept doc's explicit suggestion that Neon Tape Stop become "a component inside VHS and RetroFX."
2. Wire `GenerativeEngine`'s scheduling and seeded randomization behind a single **GENERATE** button and BPM sync toggle.
3. UI pass using the Phase 1 visual system; GENERATE gets the hero-knob/button treatment shared with VHS's DEGRADE.

**Acceptance:** Every generator produces musically usable output across its full parameter range (no generator should have "dead" regions that produce silence or pure noise unintentionally), GENERATE with a fixed seed is exactly reproducible, and Tape Sweep audibly reuses Neon Tape Stop's character rather than sounding like a separate implementation. Per the established pattern: short UI checkpoint against the mockup plus a Windows standalone smoke test before moving to NightDrive.

---

## 3. NightDrive — build third

**Concept:** a generative ambient bed generator for synthwave backdrops — a drone/texture engine that sits underneath a track instead of requiring a sample library.

**Modules (composed from existing `ff360_dsp_core` pieces plus `GenerativeEngine`):**
- Drone Engine — built on `ModulationEngine` (multi-oscillator layered pads), not a new oscillator bank.
- Granular Texture — a new lightweight granular playback component; this is the one piece of genuinely new DSP in this plugin, so give it its own unit tests.
- Arpeggiated Motion — an `IGenerator` implementation using `GenerativeEngine`'s scheduling.
- Filter Movement — slow LFO-driven filter sweeps, reusing `ModulationEngine`'s LFO primitives.
- Reverb Wash — an internal send into `ReverbEngine`, not a bundled second reverb implementation.
- Chord/Scale Lock — constrains generative pitch content to a key/scale; reads live chord data from ChordFlow when available (soft dependency), otherwise falls back to a fixed internal scale table.
- Density, Evolve Rate — direct exposure of `GenerativeEngine`'s density/evolve-rate pair.

**Build order:**
1. Build the Granular Texture component and its unit tests first, since it's the only new low-level DSP in this phase.
2. Compose Drone Engine, Filter Movement, and Reverb Wash from existing modules — this should be mostly wiring, not new algorithms.
3. Build Chord/Scale Lock with the ChordFlow soft-dependency: detect ChordFlow's output at runtime (MIDI or shared-state, whichever ChordFlow already exposes) and fall back gracefully with a warning state in the UI (not a crash or silent wrong-key output) if it's absent.
4. Wire the **EVOLVE** macro through `MacroSystem`, matching DEGRADE and GENERATE's mapping-curve approach.
5. UI pass using the Phase 1 visual system.

**Acceptance:** NightDrive runs correctly with ChordFlow absent (fixed scale table, clear UI indication that Lock is in fallback mode) and with ChordFlow present (live chord tracking, audibly following chord changes), EVOLVE produces a coherent slow drift from static to fully evolving, and Granular Texture has no audible artifacts (zipper noise, clicks) across the Density range. Per the established pattern: short UI checkpoint against the mockup plus a Windows standalone smoke test.

---

## 4. Collection-wide close-out pass

- Confirm all eight Phase 1–3 plugins link against the same `ff360_dsp_core` build with no forked copies.
- Run the full automation/CPU/latency benchmark suite (from the Phase 1 testing step) across all eight plugins for a final consistency check.
- Screenshot pass across the full collection to confirm the visual system (palette, hero-knob treatment, metering, panel style) is identical everywhere, especially the three macro knobs — DEGRADE, GENERATE, EVOLVE — which should be visually indistinguishable from each other apart from their label.
- Build a combined preset browser or at minimum a consistent preset-naming convention across all eight plugins, since they now share one preset format via `ParameterManager`.
- Final `MeteringBridge` accuracy spot-check (True Peak, RMS, LRA gate) across every plugin in the collection that shows a meter — confirms the post-Phase-10 fixes held through all three build phases and didn't drift as new plugins were wired in.
- Confirm every plugin passed its own Windows/WASAPI standalone smoke test during its build step (Steps 2–4 across all three phases) — this pass is a final roll-up check, not the first time any plugin sees Windows.

---

## Phase 3 deliverable summary

| Plugin | VST3 | AU | New/shared core module |
|---|---|---|---|
| RetroFX | ✓ | ✓ | `GenerativeEngine` (new), reuses Neon Tape Stop's state machine |
| NightDrive | ✓ | ✓ | `GenerativeEngine`, `ModulationEngine`, `ReverbEngine` + new Granular Texture component |

With Phase 3 complete, the full **ff360_labs Synthwave Production Collection** — eight plugins across VST3/AU — runs on a single shared DSP core (`ff360_dsp_core`) and one visual design system, rather than eight independently built plugins.
