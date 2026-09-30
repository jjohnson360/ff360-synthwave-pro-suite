#pragma once

#include "DesignTokens.h"
#include "Fonts.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>

namespace ff360_ui {

class FF360_MeterView : public juce::Component {
public:
    FF360_MeterView() = default;

    void setLevels(float peakL, float peakR, float rmsL, float rmsR) {
        m_peakL = peakL;
        m_peakR = peakR;
        m_rmsL = rmsL;
        m_rmsR = rmsR;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat();
        const float meterWidth = (bounds.getWidth() - 6.0f) * 0.5f;
        const float meterHeight = bounds.getHeight() - 16.0f;

        // Background slot
        g.setColour(juce::Colour(0xFF0F0F12));
        g.fillRoundedRectangle(0, 0, bounds.getWidth(), meterHeight, 3.0f);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawRoundedRectangle(0, 0, bounds.getWidth(), meterHeight, 3.0f, 1.0f);

        auto drawBar = [&](float x, float peakDb, float rmsDb) {
            // Normalize dB: -60 dB to 0 dB mapped to 0.0 to 1.0
            const float peakNorm = std::max(0.0f, std::min(1.0f, (peakDb + 60.0f) / 60.0f));
            const float rmsNorm = std::max(0.0f, std::min(1.0f, (rmsDb + 60.0f) / 60.0f));

            const float barH = rmsNorm * meterHeight;
            const float barY = meterHeight - barH;

            // RMS Bar (Amber to Gold gradient)
            juce::ColourGradient grad(juce::Colour(Colors::MetallicGold), x, meterHeight,
                                      juce::Colour(Colors::WarmAmberRed), x, 0, false);
            g.setGradientFill(grad);
            g.fillRect(x, barY, meterWidth, barH);

            // Peak Indicator line
            const float peakY = meterHeight - peakNorm * meterHeight;
            g.setColour(juce::Colour(peakDb >= -0.1f ? Colors::WarmAmberRed : Colors::TextOffWhite));
            g.fillRect(x, peakY - 1.0f, meterWidth, 2.0f);
        };

        drawBar(1.0f, m_peakL, m_rmsL);
        drawBar(meterWidth + 5.0f, m_peakR, m_rmsR);

        // Labels
        g.setColour(juce::Colour(Colors::TextDim));
        g.setFont(ff360_ui::brandFont(9.0f, juce::Font::plain));
        g.drawText("L", 1.0f, bounds.getHeight() - 14.0f, meterWidth, 12.0f, juce::Justification::centred);
        g.drawText("R", meterWidth + 5.0f, bounds.getHeight() - 14.0f, meterWidth, 12.0f, juce::Justification::centred);
    }

private:
    float m_peakL = -100.0f;
    float m_peakR = -100.0f;
    float m_rmsL = -100.0f;
    float m_rmsR = -100.0f;
};

} // namespace ff360_ui

#endif
