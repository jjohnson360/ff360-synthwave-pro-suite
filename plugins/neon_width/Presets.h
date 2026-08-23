#pragma once

#include <string>
#include <vector>

namespace ff360 {

inline std::vector<Preset> getNeonWidthPresets() {
    return {
        {
            "Wide Synth Pad", "Pads",
            R"({
  "version": 1,
  "parameters": {
    "microdelay": 4.5, "haas": 40.0, "detune": 25.0, "mswidth": 140.0,
    "freqwidth": 60.0, "freqcrossover": 450.0, "rotation": 0.0, "bassmono": 140.0,
    "mix": 100.0
  }
})"
        },
        {
            "Haas Lead Spread", "Leads",
            R"({
  "version": 1,
  "parameters": {
    "microdelay": 8.0, "haas": 65.0, "detune": 15.0, "mswidth": 120.0,
    "freqwidth": 40.0, "freqcrossover": 600.0, "rotation": 5.0, "bassmono": 180.0,
    "mix": 100.0
  }
})"
        },
        {
            "80s Club Imager", "MixBus",
            R"({
  "version": 1,
  "parameters": {
    "microdelay": 2.0, "haas": 20.0, "detune": 10.0, "mswidth": 125.0,
    "freqwidth": 80.0, "freqcrossover": 500.0, "rotation": 0.0, "bassmono": 120.0,
    "mix": 100.0
  }
})"
        },
        {
            "Mono-Safe Sub Bass", "Bass",
            R"({
  "version": 1,
  "parameters": {
    "microdelay": 0.0, "haas": 0.0, "detune": 0.0, "mswidth": 110.0,
    "freqwidth": 50.0, "freqcrossover": 350.0, "rotation": 0.0, "bassmono": 220.0,
    "mix": 100.0
  }
})"
        },
        {
            "Rotating Horizon", "Movement",
            R"({
  "version": 1,
  "parameters": {
    "microdelay": 6.0, "haas": 50.0, "detune": 45.0, "mswidth": 160.0,
    "freqwidth": 75.0, "freqcrossover": 400.0, "rotation": 25.0, "bassmono": 150.0,
    "mix": 100.0
  }
})"
        }
    };
}

} // namespace ff360
