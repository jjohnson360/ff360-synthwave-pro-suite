#pragma once

#include <string>
#include <vector>
#include "ff360/Preset.h"

namespace ff360 {

inline std::vector<Preset> getVhsPresets() {
    return {
        {
            "Clean Master Tape", "Subtle",
            R"({
  "version": 1,
  "parameters": {
    "sat": 15.0, "wow": 8.0, "flutter": 5.0, "drift": 10.0,
    "noise": 5.0, "hiss": 8.0, "dropouts": 0.0, "lofi": 0.0,
    "bitcrush": 0.0, "hfloss": 12.0, "stereodrift": 10.0, "warble": 5.0,
    "degrade": 10.0, "inGain": 0.0, "outGain": 0.0, "mix": 100.0
  }
})"
        },
        {
            "1984 VCR Broadcast", "Vintage",
            R"({
  "version": 1,
  "parameters": {
    "sat": 45.0, "wow": 40.0, "flutter": 35.0, "drift": 30.0,
    "noise": 35.0, "hiss": 45.0, "dropouts": 20.0, "lofi": 15.0,
    "bitcrush": 10.0, "hfloss": 55.0, "stereodrift": 40.0, "warble": 35.0,
    "degrade": 45.0, "inGain": 2.0, "outGain": -1.0, "mix": 100.0
  }
})"
        },
        {
            "Melancholy Cassette", "Lo-Fi",
            R"({
  "version": 1,
  "parameters": {
    "sat": 38.0, "wow": 55.0, "flutter": 25.0, "drift": 65.0,
    "noise": 40.0, "hiss": 50.0, "dropouts": 15.0, "lofi": 25.0,
    "bitcrush": 15.0, "hfloss": 60.0, "stereodrift": 50.0, "warble": 45.0,
    "degrade": 50.0, "inGain": 1.0, "outGain": 0.0, "mix": 100.0
  }
})"
        },
        {
            "Broken Walkman", "Extreme",
            R"({
  "version": 1,
  "parameters": {
    "sat": 65.0, "wow": 80.0, "flutter": 60.0, "drift": 75.0,
    "noise": 60.0, "hiss": 65.0, "dropouts": 70.0, "lofi": 50.0,
    "bitcrush": 45.0, "hfloss": 75.0, "stereodrift": 70.0, "warble": 65.0,
    "degrade": 85.0, "inGain": 4.0, "outGain": -3.0, "mix": 100.0
  }
})"
        },
        {
            "Tape Bloom & Warmth", "Harmonics",
            R"({
  "version": 1,
  "parameters": {
    "sat": 70.0, "wow": 12.0, "flutter": 8.0, "drift": 15.0,
    "noise": 15.0, "hiss": 20.0, "dropouts": 0.0, "lofi": 0.0,
    "bitcrush": 0.0, "hfloss": 25.0, "stereodrift": 20.0, "warble": 10.0,
    "degrade": 25.0, "inGain": 3.0, "outGain": -2.0, "mix": 100.0
  }
})"
        },
        {
            "VHS Sunbake", "Degraded",
            R"({
  "version": 1,
  "parameters": {
    "sat": 85.0, "wow": 70.0, "flutter": 65.0, "drift": 80.0,
    "noise": 70.0, "hiss": 75.0, "dropouts": 55.0, "lofi": 40.0,
    "bitcrush": 35.0, "hfloss": 80.0, "stereodrift": 75.0, "warble": 60.0,
    "degrade": 90.0, "inGain": 3.0, "outGain": -2.0, "mix": 100.0
  }
})"
        }
    };
}

} // namespace ff360
