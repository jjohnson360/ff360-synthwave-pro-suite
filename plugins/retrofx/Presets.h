#pragma once

#include <string>
#include <vector>

namespace ff360 {

inline std::vector<Preset> getRetroFXPresets() {
    return {
        {
            "Cyberpunk 4-Bar Riser", "Risers",
            R"({
  "version": 1,
  "parameters": {
    "generator": 5.0, "sync": 4.0, "intensity": 85.0, "seed": 42.0, "mix": 100.0
  }
})"
        },
        {
            "Analog Tape Wind-Down", "Tape",
            R"({
  "version": 1,
  "parameters": {
    "generator": 8.0, "sync": 3.0, "intensity": 90.0, "seed": 101.0, "mix": 100.0
  }
})"
        },
        {
            "Neon Laser Impact", "Impacts",
            R"({
  "version": 1,
  "parameters": {
    "generator": 2.0, "sync": 1.0, "intensity": 100.0, "seed": 777.0, "mix": 100.0
  }
})"
        },
        {
            "Dark Retro Downlifter", "Downlifters",
            R"({
  "version": 1,
  "parameters": {
    "generator": 6.0, "sync": 4.0, "intensity": 80.0, "seed": 2049.0, "mix": 100.0
  }
})"
        },
        {
            "80s White Noise Swell", "Noise",
            R"({
  "version": 1,
  "parameters": {
    "generator": 0.0, "sync": 3.0, "intensity": 75.0, "seed": 1984.0, "mix": 100.0
  }
})"
        }
    };
}

} // namespace ff360
