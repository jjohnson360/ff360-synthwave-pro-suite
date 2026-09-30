# ff360_synthwave_pro_suite

## Overview
The FF360 Synthwave Production Suite is a collection of 8 premium VST3/AU audio plugins explicitly designed for Synthwave, Darksynth, and Cyberpunk music production.

### Included Plugins:
1. **VHS (Vintage Harmonic Saturator)**
2. **Neon Chorus**
3. **Midnight Reverb**
4. **Neon Width**
5. **RetroFX**
6. **NightDrive**
7. **Neon Tape Stop**
8. **Cyberpunk Glitch**

## Build Instructions (Beta)
The plugins are built using JUCE 8 and CMake.

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## UI and Design System
All interfaces use the `ff360_ui_core` design system, featuring procedural vector graphics, neon glow effects, and a unified aesthetic tailored for 80s retro-futurism.

Editors are resizable from 75% to 200% (aspect ratio locked); each plugin remembers its size with the session.

## Credits & Licenses
- **Typography:** Barlow Condensed (Regular, SemiBold), designed by Jeremy Tribby, licensed under the [SIL Open Font License 1.1](ff360_ui_core/fonts/BarlowCondensed-OFL.txt). Embedded in each plugin.
- **Audio framework:** [JUCE](https://juce.com/).
