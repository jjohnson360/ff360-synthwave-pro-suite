#pragma once

#include <string>
#include <vector>

namespace ff360 {

struct Preset {
    std::string name;
    std::string category;
    std::string jsonContent;
};

inline std::vector<Preset> getNeonTapeStopPresets() {
    return {
        {
            "Classic 1/2 Bar Tape Brake", "Tape",
            R"({
  "version": 1,
  "parameters": {
    "slowdown": 0.80, "pitchcurve": 50.0, "filtermove": 75.0,
    "recovery": 0.30, "profile": 1.0, "reverse": 0.0, "mix": 100.0
  }
})"
        },
        {
            "Vinyl Turntable Power-Down", "Vinyl",
            R"({
  "version": 1,
  "parameters": {
    "slowdown": 1.40, "pitchcurve": 40.0, "filtermove": 60.0,
    "recovery": 0.50, "profile": 0.0, "reverse": 0.0, "mix": 100.0
  }
})"
        },
        {
            "Quick 1/16 Stutter Stop", "Fast",
            R"({
  "version": 1,
  "parameters": {
    "slowdown": 0.12, "pitchcurve": 65.0, "filtermove": 80.0,
    "recovery": 0.08, "profile": 2.0, "reverse": 0.0, "mix": 100.0
  }
})"
        },
        {
            "Reverse Tape Spool", "Reverse",
            R"({
  "version": 1,
  "parameters": {
    "slowdown": 1.00, "pitchcurve": 50.0, "filtermove": 70.0,
    "recovery": 0.60, "profile": 1.0, "reverse": 1.0, "mix": 100.0
  }
})"
        },
        {
            "Digital Freeze Stop", "Digital",
            R"({
  "version": 1,
  "parameters": {
    "slowdown": 0.25, "pitchcurve": 70.0, "filtermove": 40.0,
    "recovery": 0.20, "profile": 2.0, "reverse": 0.0, "mix": 100.0
  }
})"
        }
    };
}

} // namespace ff360
