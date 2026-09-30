#pragma once

#include "DesignTokens.h"
#include "Fonts.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>

namespace ff360_ui {

class FF360_GlassPanel : public juce::Component {
public:
    FF360_GlassPanel(const juce::String& title = "") : m_title(title) {}

    void setTitle(const juce::String& title) {
        m_title = title;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat();

        // Translucent dark charcoal background
        juce::ColourGradient bgGrad(juce::Colour(0xDD17171A), 0, 0,
                                    juce::Colour(0xEE111114), 0, bounds.getHeight(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, static_cast<float>(Spacing::PanelCornerRadius));

        // Thin Gold hairline border (0.15 alpha)
        g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.18f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), static_cast<float>(Spacing::PanelCornerRadius), 1.0f);

        // Header Title if specified
        if (m_title.isNotEmpty()) {
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.setFont(ff360_ui::brandFont(10.0f, juce::Font::bold));
            g.drawText(m_title.toUpperCase(), 12, 8, static_cast<int>(bounds.getWidth() - 24), 14, juce::Justification::left);
        }
    }

private:
    juce::String m_title;
};

} // namespace ff360_ui

#endif
