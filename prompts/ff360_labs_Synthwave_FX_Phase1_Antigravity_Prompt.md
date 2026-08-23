# ff360_labs Synthwave FX Suite — Phase 1: Core FX
### Antigravity Build Prompt

**Project:** ff360_labs Synthwave Production Collection
**Phase:** 1 of 3 — Core FX (VHS, Neon Chorus, Midnight Reverb)
**Goal of this phase:** stand up the shared DSP core, ship VHS as the flagship plugin, then Neon Chorus and Midnight Reverb on top of the same framework. This phase also establishes the visual design system every later plugin (Phase 2, Phase 3) will reuse.
**Targets:** VST3 + AU, sharing one DSP core.

---

## 0. Ground rules for the agent

- Build in the step order below. Do not start VHS until Step 1 (shared DSP core) compiles and passes its unit tests.
- Every DSP module gets a standalone unit test before it's wired into a plugin UI.
- Follow the ff360_labs naming convention: `FF360_DSP_`, `FF360_UI_`, `FF360_PARAM_` prefixes for shared classes, matching the project's existing Unity/Blender asset-naming discipline.
- Use JUCE (C++20) for both VST3 and AU, built from the same `ff360_dsp_core` library and the same JUCE `AudioProcessor` — VST3 and AU should differ only in JUCE's own export format, not in application code.
- Scope note: this collection targets VST3/AU only. Max for Live is intentionally out of scope for the whole suite — don't build a Max external, `gen~` patch, or `jsui` component for any plugin in Phases 1–3.
- Reuse the metering components already built for the ff360_labs Modular Metering Suite (Peak/RMS + VU modules) wherever a plugin needs a level display — don't rebuild metering from scratch. **Pin this to the post-Phase-10-consolidation version specifically** (the pass that fixed mislabeled True Peak, incorrect RMS, missing sample-rate handling, and the wrong LRA gate threshold). Reusing an earlier snapshot would silently reintroduce those bugs into every FX plugin that displays a meter, not just the Metering Suite itself.
- Apply the ff360_labs palette and glassmorphic panel style from the first UI pass onward (see Step 5) — don't bolt it on at the end.
- **Lesson from the Metering Suite's Phase 10 pass:** UI complaints and platform-specific bugs (the Windows WASAPI Phase Scope freeze, the standalone WASAPI loopback issue) only surfaced in a dedicated consolidation phase because they weren't checked per-plugin during the original build. Don't repeat that pattern here — each plugin gets its own short UI checkpoint and a Windows/WASAPI smoke test as part of its own acceptance criteria (see below), rather than deferring all of that to Phase 3's close-out pass.

---

## 1. Shared DSP Core — build this first

Everything else in the collection sits on top of this. Structure it as its own compilable library, `ff360_dsp_core`, so both the VST3 and AU builds link against one implementation.

**Repository layout:**

```
ff360_dsp_core/
  include/ff360/
    TapeEngine.h        // saturation, wow/flutter, pitch drift, hiss, dropouts, bit reduction
    ModulationEngine.h   // multi-voice LFO/chorus engine, shared by Neon Chorus + NightDrive (Phase 3)
    ReverbEngine.h        // algorithmic reverb core, shared by Midnight Reverb
    MacroSystem.h          // single-knob → multi-parameter mapping (DEGRADE / GENERATE / EVOLVE)
    ParameterManager.h      // parameter IDs, ranges, smoothing, preset (de)serialization
    MeteringBridge.h         // adapter onto the existing Metering Suite components (post-Phase-10 version — see ground rules)
  src/
    (implementations)
  tests/
    (unit tests per module, Catch2 or JUCE's UnitTest)
  CMakeLists.txt
```

**Module specs:**

- **TapeEngine** — owns Tape Saturation, Wow, Flutter, Pitch Drift, Noise, Hiss, Dropouts, Lo-Fi, Bit Reduction, High-Frequency Loss, Stereo Drift, Warble as independently toggleable processing blocks, each exposing 0–1 normalized parameters so `MacroSystem` can drive several at once.
- **ModulationEngine** — a bank of LFOs/oscillators with configurable voice count (1 for a plain chorus, 4 for Neon Chorus's Quad mode), stereo phase offsets, and a feedback path.
- **ReverbEngine** — pluggable algorithm interface (`IReverbAlgorithm`) with concrete implementations for Digital Hall, Dark Plate, Gated Room, Synth Room, Endless, and Dream, plus shared pre-delay, ducking, and low/high-end damping stages that sit outside the algorithm itself.
- **MacroSystem** — takes a single 0–1 input and a list of `(parameterId, curve)` mappings; used for VHS's DEGRADE, RetroFX's GENERATE (Phase 3), and NightDrive's EVOLVE (Phase 3), so build the mapping/curve format generically now even though only VHS needs it this phase.
- **ParameterManager** — central registry backing the JUCE `AudioProcessorValueTreeState`, so VST3 and AU builds always read parameter IDs, ranges, and smoothing from one source of truth. Preset format should be a flat key/value structure (JSON is fine) shared across both targets.

**Build system:**

- CMake for the core library and the JUCE targets.
- Bridge header (`extern "C"`) exposing a stable C API for the Max external to call into `ff360_dsp_core` without linking C++ ABI directly.
- CI target (even a local script is fine at this stage): build core → run unit tests → build VST3 → build AU (macOS).

**Acceptance for this step:** `ff360_dsp_core` builds standalone, all module unit tests pass, and a minimal JUCE test harness plugin can load one `TapeEngine` instance and process audio with no UI. Also confirm `MeteringBridge` is linked against the post-Phase-10 Metering Suite build (spot-check True Peak, RMS, and LRA gate readings against known reference values — don't just confirm it compiles), and run the test harness once in standalone mode on Windows to catch a WASAPI-related freeze or loopback issue this early rather than after eight plugins are built on top of it.

---

## 2. VHS (flagship plugin) — build second

This is the plugin that proves the framework, so give it the most attention.

**Parameters (all route through `TapeEngine` + `MacroSystem`):**
Tape Saturation, Wow, Flutter, Pitch Drift, Noise, Hiss, Dropouts, Lo-Fi, Bit Reduction, High-Frequency Loss, Stereo Drift, Warble — plus the single **DEGRADE** macro.

**Build order within VHS:**
1. Wire `TapeEngine` into a bare JUCE `AudioProcessor`, no UI — confirm all 12 parameters process audio correctly and automate cleanly in a host.
2. Build the DEGRADE macro mapping (curve per parameter — saturation and noise should ramp faster than pitch drift and stereo drift, so full DEGRADE doesn't sound like a single dial).
3. First UI pass using the Step 5 visual system (can be built in parallel with Step 5 once the palette/component spec exists).

**Acceptance:** VHS runs identically (same audio output, bit-for-bit where feasible) across the VST3 and AU builds, and DEGRADE produces a musically coherent single-knob sweep from clean to fully degraded. Since this is the first plugin through the pipeline, treat it as the pilot for the per-plugin checkpoint pattern: a short UI walkthrough against the mockup (labels, knob behavior, meter readout) before moving to Neon Chorus, plus a Windows standalone smoke test — don't wait for Phase 3's close-out pass to find platform issues.

---

## 3. Neon Chorus — build third

Built on `ModulationEngine`.

**Parameters:** Rate, Depth, Width, Detune, Feedback, Pre-delay, Mix, Bass Mono, Vintage/Modern, plus **Quad Chorus** mode (4-voice `ModulationEngine` configuration vs. the default 2-voice).

**Acceptance:** A/B Vintage vs. Modern modes are audibly distinct (Vintage should lean on subtle pitch instability from `TapeEngine`'s wow/flutter primitives, reused rather than reimplemented), and Quad Chorus is meaningfully wider than the default mode in a stereo field analyzer. Same per-plugin checkpoint as VHS: short UI walkthrough against the mockup, plus a Windows standalone smoke test, before moving to Midnight Reverb.

---

## 4. Midnight Reverb — build fourth

Built on `ReverbEngine`.

**Parameters:** Algorithm select (Digital Hall, Dark Plate, Gated Room, Synth Room, Endless, Dream) + Pre-delay sync, Ducking, Width, Modulation, Low-end damping, High-end damping, Freeze.

**Acceptance:** Freeze holds the current reverb tail indefinitely with no dropout, Ducking responds to input transients within one buffer's latency, and Pre-delay sync locks correctly to host tempo. Same per-plugin checkpoint as VHS and Neon Chorus: short UI walkthrough against the mockup, plus a Windows standalone smoke test.

---

## 5. Visual Design System — build alongside Step 2–4

This is the JUCE reskin phase referenced in the concept doc ("These establish your DSP framework and visual design system"). Treat it as its own workstream, not a final pass — the token source and component spec below get built once, up front, but each plugin's actual UI gets its own checkpoint (see the Acceptance line in Steps 2–4) rather than one bulk visual pass applied to all three plugins at the very end.

- **Palette:** Deep Black `#0a0a0b` (background), Matte Charcoal `#17171a` (panels), Metallic Gold `#c9a15a` (primary accent / macro knobs), Warm Amber-Red `#e8654a` (secondary accent / meters), Accessible Sky Blue `#38bdf8` (interactive/focus states only).
- **Component spec:** glassmorphic panels (translucent charcoal, subtle inner glow, thin gold hairline borders); a "hero" macro knob style shared by DEGRADE (this phase), GENERATE and EVOLVE (Phase 3); metering that visually matches the Modular Metering Suite.
- **Shared token source:** define the palette, spacing scale, and type scale once (a JSON or header-constants file) that a single JUCE `LookAndFeel` subclass reads from, so VST3 and AU never visually drift from each other.
- **Acceptance:** VHS, Neon Chorus, and Midnight Reverb all pass a side-by-side screenshot check — same palette, same knob style, same panel treatment.

---

## 6. Testing & preset pass — build last for this phase

Per-plugin UI checkpoints and Windows/WASAPI smoke tests already happened in Steps 2–4, so this pass is about collection-level consistency rather than catching first-time platform or UI bugs.

- Full automation sweep test per plugin (every parameter, host automation doesn't glitch or zipper).
- CPU/latency benchmark for each plugin at 44.1k/48k/96k.
- A starter preset bank per plugin (5–10 presets) that demonstrates the range from subtle to extreme.
- Confirm Phase 1's three plugins share zero duplicated DSP code — anything common should already live in `ff360_dsp_core`.
- Re-run the `MeteringBridge` accuracy spot-check (True Peak, RMS, LRA gate) across all three plugins' meter displays, not just the Step 1 core test — confirms the bridge behaves the same once wired into a real plugin signal chain, not just the test harness.

---

## Phase 1 deliverable summary

| Plugin | VST3 | AU | Shares core with |
|---|---|---|---|
| VHS | ✓ | ✓ | TapeEngine, MacroSystem |
| Neon Chorus | ✓ | ✓ | ModulationEngine |
| Midnight Reverb | ✓ | ✓ | ReverbEngine |

Once Phase 1 ships, Phase 2 (Neon Width, Neon Tape Stop, Cyberpunk Glitch) and Phase 3 (RetroFX, NightDrive) both build entirely on `ff360_dsp_core` — no new shared infrastructure should be needed, only new modules (`StereoEngine`, `GlitchEngine`, `GenerativeEngine`) added alongside the existing ones.
