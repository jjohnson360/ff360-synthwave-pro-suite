#pragma once

#include "DesignTokens.h"
#include "Fonts.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>

namespace ff360_ui {

class FF360_StereoFieldView : public juce::Component {
public:
    FF360_StereoFieldView() = default;

    void updateData(float phaseCorrelation, float balance, const float* leftSamples = nullptr, const float* rightSamples = nullptr, size_t numSamples = 0) {
        m_phaseCorrelation = std::max(-1.0f, std::min(1.0f, phaseCorrelation));
        m_balance = std::max(-1.0f, std::min(1.0f, balance));

        if (leftSamples && rightSamples && numSamples > 0) {
            const size_t take = std::min(numSamples, static_cast<size_t>(128));
            m_historyL.resize(take);
            m_historyR.resize(take);
            std::copy(leftSamples, leftSamples + take, m_historyL.begin());
            std::copy(rightSamples, rightSamples + take, m_historyR.begin());
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat();
        const float corrH = 18.0f;
        const auto scopeBounds = bounds.withTrimmedBottom(corrH + 6.0f);
        const float centreX = scopeBounds.getCentreX();
        const float centreY = scopeBounds.getCentreY();
        const float radius = std::min(scopeBounds.getWidth(), scopeBounds.getHeight()) * 0.5f - 8.0f;

        // Background circular frame
        g.setColour(juce::Colour(0xFF0C0C0F));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Circular grid rings
        g.setColour(juce::Colour(Colors::TrackBackground));
        g.drawEllipse(centreX - radius * 0.5f, centreY - radius * 0.5f, radius, radius, 1.0f);
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Diagonal 45-deg L/R and Mid/Side axes
        g.setColour(juce::Colour(0x18FFFFFF));
        g.drawLine(centreX - radius * 0.707f, centreY - radius * 0.707f, centreX + radius * 0.707f, centreY + radius * 0.707f, 1.0f);
        g.drawLine(centreX - radius * 0.707f, centreY + radius * 0.707f, centreX + radius * 0.707f, centreY - radius * 0.707f, 1.0f);
        g.drawLine(centreX, centreY - radius, centreX, centreY + radius, 1.0f); // Mid vertical axis
        g.drawLine(centreX - radius, centreY, centreX + radius, centreY, 1.0f); // Side horizontal axis

        // Outer hairline gold ring
        g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.25f));
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Draw Lissajous Polar Points (rotated 45 deg so Mono is vertical Mid)
        // Mid M = (L+R)/sqrt(2) -> vertical Y, Side S = (L-R)/sqrt(2) -> horizontal X
        if (!m_historyL.empty()) {
            g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.75f));
            juce::Path p;
            bool started = false;

            for (size_t i = 0; i < m_historyL.size(); ++i) {
                const float l = m_historyL[i];
                const float r = m_historyR[i];

                const float x = centreX + ((l - r) * 0.7071f) * radius;
                const float y = centreY - ((l + r) * 0.7071f) * radius;

                if (!started) {
                    p.startNewSubPath(x, y);
                    started = true;
                } else {
                    p.lineTo(x, y);
                }
            }
            g.strokePath(p, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Horizontal Phase Correlation Meter at bottom
        const auto corrBounds = juce::Rectangle<float>(bounds.getX() + 10.0f, bounds.getBottom() - corrH, bounds.getWidth() - 20.0f, corrH);
        g.setColour(juce::Colour(0xFF121215));
        g.fillRoundedRectangle(corrBounds, 3.0f);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawRoundedRectangle(corrBounds, 3.0f, 1.0f);

        // Center zero mark
        const float cMidX = corrBounds.getCentreX();
        g.setColour(juce::Colour(Colors::TextDim));
        g.drawVerticalLine(static_cast<int>(cMidX), corrBounds.getY() + 2.0f, corrBounds.getBottom() - 2.0f);

        // Indicator Needle / Bar for correlation (-1.0 to +1.0)
        const float indX = cMidX + m_phaseCorrelation * (corrBounds.getWidth() * 0.5f - 4.0f);
        const auto indCol = (m_phaseCorrelation < -0.1f) ? juce::Colour(Colors::WarmAmberRed)
                                                         : juce::Colour(Colors::MetallicGold);
        g.setColour(indCol);
        g.fillRect(indX - 2.0f, corrBounds.getY() + 2.0f, 4.0f, corrBounds.getHeight() - 4.0f);

        // Scale labels
        g.setColour(juce::Colour(Colors::TextDim));
        g.setFont(ff360_ui::brandFont(8.5f, juce::Font::plain));
        g.drawText("-1", corrBounds.getX() + 2.0f, corrBounds.getY(), 14.0f, corrH, juce::Justification::left);
        g.drawText("0", cMidX - 6.0f, corrBounds.getY(), 12.0f, corrH, juce::Justification::centred);
        g.drawText("+1", corrBounds.getRight() - 16.0f, corrBounds.getY(), 14.0f, corrH, juce::Justification::right);
    }

private:
    float m_phaseCorrelation = 1.0f;
    float m_balance = 0.0f;
    std::vector<float> m_historyL;
    std::vector<float> m_historyR;
};

} // namespace ff360_ui

#endif
