#pragma once

#include <cstdint>

namespace ff360_ui {

struct Colors {
    static constexpr uint32_t DeepBlack       = 0xFF0A0A0B; // Main background
    static constexpr uint32_t MatteCharcoal   = 0xFF17171A; // Main panels
    static constexpr uint32_t Charcoal2       = 0xFF1E1E22; // Inner surfaces
    static constexpr uint32_t MetallicGold    = 0xFFC9A15A; // Primary accent & Hero knob
    static constexpr uint32_t GoldSoft        = 0x59C9A15A; // Translucent glow
    static constexpr uint32_t WarmAmberRed    = 0xFFE8654A; // Secondary accent / meters
    static constexpr uint32_t AccessibleSky   = 0xFF38BDF8; // Interactive / focus state
    static constexpr uint32_t TextOffWhite    = 0xFFEAE6DD; // Primary text
    static constexpr uint32_t TextDim         = 0xFF9A958A; // Secondary text / labels
    static constexpr uint32_t TrackBackground = 0x1FFFFFFF; // Arc track
};

struct Spacing {
    static constexpr int GridPadding = 16;
    static constexpr int PanelCornerRadius = 10;
    static constexpr int CardCornerRadius = 14;
    static constexpr int KnobMargin = 8;
};

} // namespace ff360_ui
