#pragma once

#include <string>
#include <vector>
#include "ff360/Preset.h"

namespace ff360 {

inline std::vector<Preset> getNightDrivePresets() {
    return {
        {
            "Midnight Highway Drone", "Drones",
            R"({
  "version": 1,
  "parameters": {
    "dronelevel": 80.0, "granularlevel": 60.0, "arplevel": 40.0,
    "evolve": 50.0, "density": 30.0, "reverbwash": 65.0,
    "scalelock": 1.0, "filtermove": 60.0, "mix": 100.0
  }
})"
        },
        {
            "Neon Rain Cloud", "Atmospheres",
            R"({
  "version": 1,
  "parameters": {
    "dronelevel": 50.0, "granularlevel": 90.0, "arplevel": 20.0,
    "evolve": 75.0, "density": 45.0, "reverbwash": 80.0,
    "scalelock": 6.0, "filtermove": 80.0, "mix": 100.0
  }
})"
        },
        {
            "Blade Runner Warm Pad", "Pads",
            R"({
  "version": 1,
  "parameters": {
    "dronelevel": 95.0, "granularlevel": 40.0, "arplevel": 10.0,
    "evolve": 35.0, "density": 20.0, "reverbwash": 70.0,
    "scalelock": 2.0, "filtermove": 45.0, "mix": 100.0
  }
})"
        },
        {
            "Endless Cybernetic Drift", "Ambient",
            R"({
  "version": 1,
  "parameters": {
    "dronelevel": 70.0, "granularlevel": 75.0, "arplevel": 55.0,
    "evolve": 85.0, "density": 40.0, "reverbwash": 85.0,
    "scalelock": 1.0, "filtermove": 90.0, "mix": 100.0
  }
})"
        },
        {
            "Tokyo 3AM Atmosphere", "Cinematic",
            R"({
  "version": 1,
  "parameters": {
    "dronelevel": 60.0, "granularlevel": 80.0, "arplevel": 35.0,
    "evolve": 60.0, "density": 35.0, "reverbwash": 75.0,
    "scalelock": 6.0, "filtermove": 50.0, "mix": 100.0
  }
})"
        }
    };
}

} // namespace ff360
