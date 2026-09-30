#pragma once

#if __has_include(<juce_graphics/juce_graphics.h>)
#include <juce_graphics/juce_graphics.h>

namespace ff360_ui {

// Barlow Condensed (the brand face, see FF360_LookAndFeelBase) draws smaller than the system
// font these sizes were designed with; this keeps the text the size the layouts expect.
inline constexpr float kBrandFontScale = 1.25f;

// A font in the brand face at a layout's design size (the size you'd have written for the
// system font): ff360_ui::brandFont(9.0f, juce::Font::bold)
inline juce::Font brandFont(float designSize, int styleFlags = juce::Font::plain) {
    return juce::Font(designSize * kBrandFontScale, styleFlags);
}

} // namespace ff360_ui

#endif
