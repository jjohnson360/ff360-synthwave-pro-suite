# ff360_labs Synthwave FX Suite — Phase 1: Core FX Implementation Plan

Phase 1 of the ff360_labs Synthwave Production Collection establishes the shared C++20 DSP core (`ff360_dsp_core`), the modular parameter and metering framework, the brand's glassmorphic UI design system, and three plugins:
1. **VHS** (Flagship tape degradation processor with DEGRADE macro)
2. **Neon Chorus** (80s stereo chorus with Vintage/Modern and Quad modes)
3. **Midnight Reverb** (Cinematic retro digital reverb with 6 algorithms, Freeze, and Ducking)

---

## Architecture Overview

```mermaid
graph TD
    subgraph Core ["ff360_dsp_core (C++20 Shared Library)"]
        TE["TapeEngine<br/>(Saturation, Wow, Flutter, Drift, Noise, Hiss, Lo-Fi, Bitcrush, HF Loss)"]
        ME["ModulationEngine<br/>(Multi-voice LFO, Stereo Phase, Detune, Feedback)"]
        RE["ReverbEngine<br/>(Digital Hall, Dark Plate, Gated, Synth Room, Endless, Dream)"]
        MS["MacroSystem<br/>(Param mapping curves: DEGRADE, GENERATE, EVOLVE)"]
        PM["ParameterManager<br/>(APVTS registry, Smoothing, Preset serialization)"]
        MB["MeteringBridge<br/>(True Peak, RMS, VU, LRA gate post-Phase-10)"]
    end

    subgraph Plugins ["Phase 1 Audio Plugins (JUCE VST3 / AU)"]
        VHS["VHS Plugin<br/>(TapeEngine + MacroSystem DEGRADE)"]
        NC["Neon Chorus Plugin<br/>(ModulationEngine + Quad Mode)"]
        MR["Midnight Reverb Plugin<br/>(ReverbEngine + Freeze + Ducking)"]
    end

    subgraph UI ["Visual Design System (JUCE LookAndFeel)"]
        LAF["FF360_LookAndFeel<br/>(Deep Black #0a0a0b, Matte Charcoal #17171a, Metallic Gold #c9a15a)"]
        KNOB["FF360_HeroKnob & StandardKnob"]
        METER["FF360_MeterComponent"]
        PANEL["FF360_GlassPanel"]
    end

    Core --> VHS
    Core --> NC
    Core --> MR
    UI --> VHS
    UI --> NC
    UI --> MR
```
