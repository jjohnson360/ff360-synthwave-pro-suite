# ff360_labs Synthwave Production Suite

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![JUCE](https://img.shields.io/badge/JUCE-7%20%2F%208-orange.svg)](https://juce.com/)
[![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20macOS%2012%2B-blueviolet.svg)](https://github.com/)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)
[![Status](https://img.shields.io/badge/Release-v1.0.0-gold.svg)](https://github.com/)

A definitive collection of eight professional audio plugins engineered specifically for modern **Synthwave, Cyberpunk, Darksynth, Vaporwave, and Retro-Futuristic** music production.

---

## 🌟 The 8-Plugin Collection

```
 ┌──────────────────────────────────────────────────────────────────────────────────┐
 │                           ff360_labs SYNTHWAVE SUITE                             │
 ├─────────────────────────┬────────────────────────────┬───────────────────────────┤
 │     PHASE 1: CORE FX    │    PHASE 2: CREATIVE FX    │   PHASE 3: GENERATIVE FX  │
 ├─────────────────────────┼────────────────────────────┼───────────────────────────┤
 │ 📼 VHS (Flagship)       │ 🌐 Neon Width              │ ⚡ RetroFX                 │
 │ 🔮 Neon Chorus          │ 🛑 Neon Tape Stop          │ 🌃 NightDrive             │
 │ 🌌 Midnight Reverb      │ 👾 Cyberpunk Glitch        │                           │
 └─────────────────────────┴────────────────────────────┴───────────────────────────┘
```

### 1. 📼 VHS — Flagship Tape Degradation Processor
- **Signature Control**: `DEGRADE` Golden-Ringed Hero Macro Knob.
- **DSP Engine**: 12 toggleable tape degradation modules with hysteresis tape saturation, wow, flutter, pitch drift, dynamic high-frequency loss, lo-fi decimation, magnetic head wear, dropouts, and mechanical hiss.
- **Factory Presets**: *Clean Master Tape, 1984 VCR Broadcast, Melancholy Cassette, Broken Walkman, Tape Bloom & Warmth, VHS Sunbake*.

### 2. 🔮 Neon Chorus — 80s Multi-Voice Chorus & Ensemble
- **Signature Control**: Stereo (2-voice) / Quad (4-voice) and Modern / Vintage character toggles.
- **DSP Engine**: Rich BBD-style multi-voice chorus with 4th-order Linkwitz-Riley Bass Mono crossover filter for 100% low-end sub clarity.
- **Factory Presets**: *Juno Pad Spread, Tokyo 1986 Ensemble, Dimension 4D Clean, Subtle Synthwave Shimmer, Rotary Pad Movement*.

### 3. 🌌 Midnight Reverb — Retro Algorithmic Spaces & Shimmer
- **Signature Control**: 6 Algorithmic Space Models with Transient Ducking Envelope & Infinite Freeze.
- **Algorithms**: *Digital Hall, Dark Plate, Gated Room, Synth Room, Endless, Dream*.
- **Factory Presets**: *Cyberpunk Cathedral, 80s Snare Gate, Dark Vintage Plate, Infinite Void, Blade Runner Dream Shimmer*.

### 4. 🌐 Neon Width — Psychoacoustic Stereo Imaging & Goniometer
- **Signature Control**: Real-time Polar / Lissajous Goniometer and Phase Correlation Meter Bar.
- **DSP Engine**: Micro-delay, Haas effect, continuous stereo detune, M/S width (0–200%), 4th-order crossover frequency width, and trigonometric rotation matrix.
- **Factory Presets**: *Wide Synth Pad, Haas Lead Spread, 80s Club Imager, Mono-Safe Sub Bass, Rotating Horizon*.

### 5. 🛑 Neon Tape Stop — Musical Analog, Vinyl & Digital Tape Brake
- **Signature Control**: Illuminated momentary `STOP` button with selectable inertia curves.
- **Profiles**: *Vinyl Stop* (S-curve motor drag), *Tape Stop* (exponential inertia + HF loss sweep), and *Digital Stop* (stepped lo-fi quantization cut).
- **Factory Presets**: *Classic 1/2 Bar Tape Brake, Vinyl Turntable Power-Down, Quick 1/16 Stutter Stop, Reverse Tape Spool, Digital Freeze Stop*.

### 6. 👾 Cyberpunk Glitch — Tempo-Synced Stutter & Buffer Glitch
- **Signature Control**: `PROBABILITY` Golden-Ringed Hero Knob with rhythm division grid (1/4 to 1/32).
- **DSP Engine**: 4-second circular buffer, beat repeat, reverse buffer playback, buffer freeze, granular pitch shift (-12 to +12 semitones), bitcrush, and resonant filter sweep.
- **Factory Presets**: *1/16 Beat Stutter, Dystopian Reverse Repeats, Octave Jump Glitch, Cyberpunk Buffer Freeze, Random Artifact Matrix*.

### 7. ⚡ RetroFX — Generative Synthwave Transition Synthesizer
- **Signature Control**: `GENERATE` Hero Macro Knob & Button with deterministic recall seed.
- **Generators (9 Types)**: *Noise Sweep, Pitch Sweep, Laser, Reverse, Impact, Riser, Downlifter, Digital Sweep, Tape Sweep*.
- **Factory Presets**: *Cyberpunk 4-Bar Riser, Analog Tape Wind-Down, Neon Laser Impact, Dark Retro Downlifter, 80s White Noise Swell*.

### 8. 🌃 NightDrive — Generative Synthwave Ambient Bed & Textures
- **Signature Control**: `EVOLVE` Golden-Ringed Hero Macro Knob + Chord / Scale Lock.
- **DSP Engine**: Composed multi-oscillator drone chorus, Hann-windowed granular texture cloud, arpeggiated movement, slow filter LFO, and Reverb wash send.
- **ChordFlow Soft-Dependency**: Real-time chord tracking with automatic graceful fallback to internal scale table (*Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Synthwave Pentatonic*).
- **Factory Presets**: *Midnight Highway Drone, Neon Rain Cloud, Blade Runner Warm Pad, Endless Cybernetic Drift, Tokyo 3AM Atmosphere*.

---

## 🏛️ Architecture & Shared DSP Core (`ff360_dsp_core`)

All eight plugins share a single, zero-duplication C++20 DSP static library and C ABI export bridge:

```
ff360_synthwave_pro_suite/
├── CMakeLists.txt
├── README.md
├── ff360_dsp_core/           # Unified C++20 DSP Library
│   ├── include/ff360/       # Public C++ Headers & C API (ff360_dsp_c_api.h)
│   ├── src/                 # High-performance DSP algorithms
│   └── tests/               # Standalone unit tests & suite benchmarks
├── ff360_ui_core/            # Visual Design System (JUCE LookAndFeel)
│   └── include/ff360_ui/    # Glassmorphism, HeroKnob, MeterView, StereoFieldView
└── plugins/                  # 8 Audio Plugin Processors & Editors
    ├── vhs/
    ├── neon_chorus/
    ├── midnight_reverb/
    ├── neon_width/
    ├── neon_tape_stop/
    ├── cyberpunk_glitch/
    ├── retrofx/
    └── nightdrive/
```

### Post-Phase-10 Precision Metering Compliance
Every plugin includes an integrated `MeteringBridge` adhering to the highest broadcast metering standards:
- **4x True Peak Oversampler**: Polyphase FIR interpolation for inter-sample peak detection.
- **RMS Energy Meter**: Sample-rate independent sliding window integrator.
- **IEC 60268-17 VU Ballistics**: Authentic analog meter needle dynamics.
- **EBU R128 Loudness Range (LRA)**: $-70\text{ dBFS}$ absolute and $-10\text{ LU}$ relative gated statistical loudness range.

---

## 💻 Building from Source

### Prerequisites
- **CMake 3.20+**
- **C++20 compliant compiler** (MSVC 2022 on Windows, Clang on macOS 12+)
- **JUCE Framework 7 or 8** (Optional: standalone DSP tests and C API build without external dependencies)

### Windows (MSVC)
```powershell
# Configure build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compile all targets, tests, and benchmarks
cmake --build build --config Release

# Run complete 8-plugin verification and benchmark runner
.\build\Release\ff360_suite_benchmarks_collection.exe
```

### macOS 12+ (Universal Binary: Apple Silicon + Intel)
```bash
# Configure Universal Build
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"

# Build Release targets
cmake --build build --config Release

# Run suite benchmarks
./build/ff360_suite_benchmarks_collection
```

---

## 📊 Benchmark & Performance Summary

Continuous real-time stream simulation results across all core modules (Block size: 256 samples):

| Module / Plugin Engine | 44.1 kHz CPU | 48.0 kHz CPU | 96.0 kHz CPU | Real-Time Headroom |
|---|---|---|---|---|
| **TapeEngine** (VHS) | 0.34% | 0.38% | 0.77% | ~130x to 295x |
| **ModulationEngine** (Neon Chorus) | 0.63% | 0.76% | 1.38% | ~72x to 157x |
| **ReverbEngine** (Midnight Reverb) | 0.31% | 0.34% | 0.69% | ~144x to 318x |
| **StereoEngine** (Neon Width) | 0.21% | 0.23% | 0.49% | ~203x to 474x |
| **GlitchEngine** (Cyberpunk Glitch) | 0.03% | 0.03% | 0.06% | ~1627x to 3658x |
| **GenerativeEngine** (RetroFX) | 0.21% | 0.22% | 0.45% | ~223x to 487x |
| **GranularTexture** (NightDrive) | 0.14% | 0.15% | 0.30% | ~333x to 720x |
| **MeteringBridge** (All Plugins) | 0.05% | 0.06% | 0.12% | ~839x to 1818x |

---

## 📄 Documentation & Manual
- **User Manual (PDF)**: Included in all releases at `manuals/ff360_Synthwave_Production_Suite_User_Manual.pdf`.
- **Phase 1 Walkthrough**: [`docs/Walkthroughs/Phase1_Core_FX_Walkthrough.md`](file:///c:/Users/fredd/OneDrive/Desktop/ff360_labs/ff360_labs%20Synthwave%20Production%20Suite/ff360_synthwave_pro_suite/docs/Walkthroughs/Phase1_Core_FX_Walkthrough.md)
- **Phase 2 Walkthrough**: [`docs/Walkthroughs/Phase2_Creative_FX_Walkthrough.md`](file:///c:/Users/fredd/OneDrive/Desktop/ff360_labs/ff360_labs%20Synthwave%20Production%20Suite/ff360_synthwave_pro_suite/docs/Walkthroughs/Phase2_Creative_FX_Walkthrough.md)
- **Phase 3 Walkthrough**: [`docs/Walkthroughs/Phase3_Generative_FX_Walkthrough.md`](file:///c:/Users/fredd/OneDrive/Desktop/ff360_labs/ff360_labs%20Synthwave%20Production%20Suite/ff360_synthwave_pro_suite/docs/Walkthroughs/Phase3_Generative_FX_Walkthrough.md)

---

## 📜 License
Copyright © 2026 ff360_labs. All rights reserved.
Proprietary software for digital audio workstations.
