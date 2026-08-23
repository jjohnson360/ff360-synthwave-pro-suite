#pragma once

#include <string>
#include <vector>

namespace ff360 {

struct Preset {
    std::string name;
    std::string category;
    std::string jsonContent;
};

inline std::vector<Preset> getMidnightReverbPresets() {
    return {
        {
            "Cyberpunk Cathedral", "Halls",
            R"({
  "version": 1,
  "parameters": {
    "alg": 0.0, "decay": 70.0, "predelay": 24.0, "synctoggle": 0.0,
    "ducking": 25.0, "width": 100.0, "modulation": 45.0, "lowdamp": 160.0,
    "highdamp": 9000.0, "freeze": 0.0, "mix": 40.0
  }
})"
        },
        {
            "80s Snare Gate", "Gated",
            R"({
  "version": 1,
  "parameters": {
    "alg": 2.0, "decay": 35.0, "predelay": 0.0, "synctoggle": 0.0,
    "ducking": 0.0, "width": 90.0, "modulation": 10.0, "lowdamp": 200.0,
    "highdamp": 12000.0, "freeze": 0.0, "mix": 60.0
  }
})"
        },
        {
            "Dark Vintage Plate", "Plates",
            R"({
  "version": 1,
  "parameters": {
    "alg": 1.0, "decay": 50.0, "predelay": 15.0, "synctoggle": 0.0,
    "ducking": 20.0, "width": 85.0, "modulation": 25.0, "lowdamp": 180.0,
    "highdamp": 4500.0, "freeze": 0.0, "mix": 40.0
  }
})"
        },
        {
            "Infinite Void", "Ambient",
            R"({
  "version": 1,
  "parameters": {
    "alg": 4.0, "decay": 95.0, "predelay": 30.0, "synctoggle": 0.0,
    "ducking": 35.0, "width": 100.0, "modulation": 30.0, "lowdamp": 120.0,
    "highdamp": 14000.0, "freeze": 1.0, "mix": 50.0
  }
})"
        },
        {
            "Blade Runner Dream Shimmer", "Cinematic",
            R"({
  "version": 1,
  "parameters": {
    "alg": 5.0, "decay": 80.0, "predelay": 20.0, "synctoggle": 1.0,
    "ducking": 45.0, "width": 100.0, "modulation": 70.0, "lowdamp": 150.0,
    "highdamp": 11000.0, "freeze": 0.0, "mix": 45.0
  }
})"
        }
    };
}

} // namespace ff360
