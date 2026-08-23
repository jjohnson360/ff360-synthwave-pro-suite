# ff360_labs Synthwave FX Suite — Phase 2: Creative FX Implementation Plan

Phase 2 builds upon the completed Phase 1 foundation (`ff360_dsp_core` and `ff360_ui_core`), adding two new DSP modules to the core library (`StereoEngine`, `GlitchEngine`), extending `TapeEngine` with the embeddable `TapeStopController`, and shipping three new creative plugins:
1. **Neon Width** (Stereo imaging & movement processor with real-time stereo field visualizer)
2. **Neon Tape Stop** (Musical tape stop with Vinyl, Tape, and Digital stop profiles)
3. **Cyberpunk Glitch** (Beat-synced rhythmic glitch, stutter, buffer freeze, reverse, and pitch shift)

---

## Architecture Overview

```mermaid
graph TD
    subgraph Core ["ff360_dsp_core (Phase 2 Additions & Reused Modules)"]
        SE["StereoEngine (NEW)<br/>(Haas, M/S, Freq Width, Rotation, Bass Mono, Correlation)"]
        GE["GlitchEngine (NEW)<br/>(Stutter, Beat Repeat, Reverse, Freeze, Pitch Shift, Probability)"]
        TSC["TapeStopController (NEW)<br/>(State machine & Stop curves over TapeEngine)"]
        TE["TapeEngine (Reused)"]
        MB["MeteringBridge (Reused)"]
        PM["ParameterManager (Reused)"]
    end

    subgraph UI ["ff360_ui_core"]
        LAF["FF360_LookAndFeel"]
        SFV["FF360_StereoFieldView (NEW)<br/>(Real-time Polar/Lissajous & Correlation Meter)"]
        GP["FF360_GlassPanel"]
        HK["FF360_HeroKnob"]
        MV["FF360_MeterView"]
    end

    subgraph Plugins ["Phase 2 Audio Plugins"]
        NW["Neon Width Plugin<br/>(StereoEngine + StereoFieldView)"]
        NTS["Neon Tape Stop Plugin<br/>(TapeStopController + TapeEngine)"]
        CG["Cyberpunk Glitch Plugin<br/>(GlitchEngine + Rhythm Grid)"]
    end

    SE --> NW
    SFV --> NW
    TSC --> NTS
    TE --> NTS
    GE --> CG
    UI --> NW
    UI --> NTS
    UI --> CG
```
