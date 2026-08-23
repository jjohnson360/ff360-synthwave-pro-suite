# ff360_labs Synthwave FX Suite — Phase 3: Generative FX Implementation Plan

Phase 3 is the final phase of the ff360_labs Synthwave Production Collection. It introduces `GenerativeEngine` and the `GranularTexture` engine to `ff360_dsp_core`, and ships the two generative plugins:
1. **RetroFX** (Generative synthwave transition and riser generator on demand with 9 generators + Hero GENERATE knob)
2. **NightDrive** (Generative ambient synthwave backdrop and drone bed engine + Hero EVOLVE knob + Chord/Scale Lock)

This phase concludes with a collection-wide audit and benchmark suite covering all 8 plugins in the suite.

---

## Architecture Overview

```mermaid
graph TD
    subgraph Core ["ff360_dsp_core (Complete 8-Module Core)"]
        TE["TapeEngine"]
        ME["ModulationEngine"]
        RE["ReverbEngine"]
        MS["MacroSystem"]
        PM["ParameterManager"]
        MB["MeteringBridge"]
        SE["StereoEngine"]
        GE["GlitchEngine"]
        TSC["TapeStopController"]
        GEN["GenerativeEngine (NEW)<br/>(IGenerator, Seeded PRNG, BPM Scheduling, Density/Evolve)"]
        GT["GranularTexture (NEW)<br/>(Grain cloud synthesis & Hann window smoothing)"]
    end

    subgraph Phase3Plugins ["Phase 3 Audio Plugins"]
        RFX["RetroFX Plugin<br/>(Generative Transitions: 9 IGenerators + GENERATE Macro)"]
        ND["NightDrive Plugin<br/>(Ambient Bed: Drone + Granular + Arp + Reverb Wash + EVOLVE)"]
    end

    GEN --> RFX
    TSC --> RFX
    GEN --> ND
    GT --> ND
    ME --> ND
    RE --> ND
    MS --> ND
    MB --> RFX
    MB --> ND
```
