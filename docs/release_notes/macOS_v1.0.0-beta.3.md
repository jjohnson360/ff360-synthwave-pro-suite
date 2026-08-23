# ff360_labs Synthwave Production Suite — v1.0.0-beta.3 (macOS Universal Release)

## Overview
This is a **brand-new genuine macOS 12+ Universal binary release** (supporting both Apple Silicon `arm64` and Intel `x86_64` processors). It contains all 8 audio effects plugins formatted as genuine macOS **VST3** (`.vst3`) and **Audio Unit** (`.component`) directory bundles.

## Included Plugins (All 8 FX):
1. **VHS** (`Vhs0` / `FF36`) — Vintage tape saturation, flutter, hiss, mechanical wow, and degrade macro.
2. **Neon Chorus** (`NCh0` / `FF36`) — Classic 80s BBD ensemble chorus with Vintage/Modern analog modeling and Quad spread.
3. **Midnight Reverb** (`MRev` / `FF36`) — Lush algorithmic 80s digital reverb (6 modes: Digital Hall, Dark Plate, Gated Room, Synth Room, Endless, Dream Shimmer).
4. **Neon Width** (`NWdt` / `FF36`) — Dimension expander, Haas stereo wideness, and mono-compatible bass management.
5. **Neon Tape Stop** (`NTSt` / `FF36`) — Smooth tape deceleration and acceleration stop simulation.
6. **Cyberpunk Glitch** (`CGlt` / `FF36`) — Stutter buffer repeats, bitcrush degradation, and sync tempo glitching.
7. **RetroFX** (`RFX0` / `FF36`) — Vintage colorizer and lo-fi tonal shaping.
8. **NightDrive** (`NDrv` / `FF36`) — Synthwave dynamic drive, saturation, and tube excitement.

---

## Installation Instructions (macOS)

### 1. Extract Archive
Unpack `ff360_Synthwave_FX_Suite_v1.0.0-beta.3_macOS_Universal.zip`.

### 2. Copy Plugin Bundles to System Directories
- **VST3 Plugins (`.vst3`)**:
  Copy all `.vst3` bundles into:
  ```bash
  /Library/Audio/Plug-Ins/VST3/
  # or for user-only:
  ~/Library/Audio/Plug-Ins/VST3/
  ```
- **Audio Unit Plugins (`.component`)**:
  Copy all `.component` bundles into:
  ```bash
  /Library/Audio/Plug-Ins/Components/
  # or for user-only:
  ~/Library/Audio/Plug-Ins/Components/
  ```

---

## Gatekeeper / Ad-Hoc Code Signing Note
These plugins are compiled as universal binaries and signed with ad-hoc signatures for beta testing. Because they are not notarized through a commercial Apple Developer ID, macOS Gatekeeper may quarantine downloaded files.

If macOS displays *"Plugin cannot be opened because the developer cannot be verified"*, open Terminal and run:

```bash
# Clear quarantine flag on VST3 plugins:
sudo xattr -cr /Library/Audio/Plug-Ins/VST3/*.vst3
# or user directory:
xattr -cr ~/Library/Audio/Plug-Ins/VST3/*.vst3

# Clear quarantine flag on AU plugins:
sudo xattr -cr /Library/Audio/Plug-Ins/Components/*.component
# or user directory:
xattr -cr ~/Library/Audio/Plug-Ins/Components/*.component

# Reset Audio Component cache for Logic Pro / GarageBand / Ableton:
killall -9 AudioComponentRegistrar
```

After clearing quarantine, rescan plugins in your DAW (Logic Pro, Ableton Live, Reaper, Studio One, Bitwig, Cubase, FL Studio).
