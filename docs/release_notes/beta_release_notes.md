# ff360_labs Synthwave Production Suite — Beta Release

## Overview
This release contains all 8 audio effects plugins for **macOS 12+ Universal** (Apple Silicon `arm64` + Intel `x86_64`) and **Windows x64**, formatted as genuine **VST3** (`.vst3`) bundles, plus **Audio Unit** (`.component`) bundles on macOS.

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
Unpack `ff360_Synthwave_FX_Suite_<version>_macOS_Universal.zip`.

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

### Gatekeeper / Ad-Hoc Code Signing Note
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

---

## Installation Instructions (Windows)

### 1. Extract Archive
Unpack `ff360_Synthwave_FX_Suite_<version>_Windows_x64.zip`.

### 2. Copy Plugin Bundles to System Directory
Copy all `.vst3` bundles into:
```
C:\Program Files\Common Files\VST3\
```

### 3. Rescan in Your DAW
Trigger a plugin rescan in your DAW (Ableton Live, Reaper, Studio One, Bitwig, Cubase, FL Studio). If your DAW caches plugin metadata and the new version doesn't appear, force a full rescan rather than an incremental one.

---

## Notes
- These are beta builds, provided as-is for testing and feedback.
- macOS builds are ad-hoc signed only (not notarized) — see the Gatekeeper note above.
- Windows builds are unsigned — Windows SmartScreen may warn on first extraction; this is expected for an unsigned beta binary.
