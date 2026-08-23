#pragma once

#include <string>
#include <vector>
#include "ff360/Preset.h"

namespace ff360 {

inline std::vector<Preset> getNeonChorusPresets() {
    return {
        {
            "Juno Pad Spread", "Pads",
            R"({
  "version": 1,
  "parameters": {
    "rate": 0.75, "depth": 65.0, "width": 100.0, "detune": 30.0,
    "feedback": 0.0, "predelay": 4.0, "mix": 50.0, "bassmono": 140.0,
    "vintage": 1.0, "quad": 1.0
  }
})"
        },
        {
            "Tokyo 1986 Ensemble", "Vintage",
            R"({
  "version": 1,
  "parameters": {
    "rate": 1.40, "depth": 75.0, "width": 100.0, "detune": 45.0,
    "feedback": 15.0, "predelay": 6.0, "mix": 60.0, "bassmono": 160.0,
    "vintage": 1.0, "quad": 1.0
  }
})"
        },
        {
            "Dimension 4D Clean", "Modern",
            R"({
  "version": 1,
  "parameters": {
    "rate": 0.45, "depth": 50.0, "width": 100.0, "detune": 25.0,
    "feedback": 5.0, "predelay": 3.0, "mix": 50.0, "bassmono": 120.0,
    "vintage": 0.0, "quad": 1.0
  }
})"
        },
        {
            "Subtle Synthwave Shimmer", "Subtle",
            R"({
  "version": 1,
  "parameters": {
    "rate": 0.30, "depth": 35.0, "width": 80.0, "detune": 15.0,
    "feedback": 0.0, "predelay": 2.0, "mix": 40.0, "bassmono": 100.0,
    "vintage": 1.0, "quad": 0.0
  }
})"
        },
        {
            "Rotary Pad Movement", "Creative",
            R"({
  "version": 1,
  "parameters": {
    "rate": 3.50, "depth": 80.0, "width": 100.0, "detune": 50.0,
    "feedback": 35.0, "predelay": 8.0, "mix": 70.0, "bassmono": 180.0,
    "vintage": 0.0, "quad": 1.0
  }
})"
        }
    };
}

} // namespace ff360
