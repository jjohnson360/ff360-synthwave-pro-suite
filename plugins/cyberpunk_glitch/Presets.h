#pragma once

#include <string>
#include <vector>
#include "ff360/Preset.h"

namespace ff360 {

inline std::vector<Preset> getCyberpunkGlitchPresets() {
    return {
        {
            "1/16 Beat Stutter", "Rhythm",
            R"({
  "version": 1,
  "parameters": {
    "division": 2.0, "probability": 60.0, "reverse": 0.0, "freeze": 0.0,
    "pitch": 0.0, "bitcrush": 0.0, "gate": 80.0, "filter": 16000.0,
    "resonance": 0.707, "mix": 50.0
  }
})"
        },
        {
            "Dystopian Reverse Repeats", "Dark",
            R"({
  "version": 1,
  "parameters": {
    "division": 1.0, "probability": 75.0, "reverse": 1.0, "freeze": 0.0,
    "pitch": -12.0, "bitcrush": 35.0, "gate": 100.0, "filter": 8000.0,
    "resonance": 1.2, "mix": 60.0
  }
})"
        },
        {
            "Octave Jump Glitch", "Pitch",
            R"({
  "version": 1,
  "parameters": {
    "division": 3.0, "probability": 50.0, "reverse": 0.0, "freeze": 0.0,
    "pitch": 12.0, "bitcrush": 15.0, "gate": 50.0, "filter": 12000.0,
    "resonance": 2.5, "mix": 60.0
  }
})"
        },
        {
            "Cyberpunk Buffer Freeze", "Freeze",
            R"({
  "version": 1,
  "parameters": {
    "division": 2.0, "probability": 100.0, "reverse": 0.0, "freeze": 1.0,
    "pitch": 0.0, "bitcrush": 20.0, "gate": 100.0, "filter": 6000.0,
    "resonance": 1.8, "mix": 70.0
  }
})"
        },
        {
            "Random Artifact Matrix", "Artifacts",
            R"({
  "version": 1,
  "parameters": {
    "division": 2.0, "probability": 35.0, "reverse": 1.0, "freeze": 0.0,
    "pitch": -5.0, "bitcrush": 60.0, "gate": 65.0, "filter": 4500.0,
    "resonance": 2.0, "mix": 55.0
  }
})"
        }
    };
}

} // namespace ff360
