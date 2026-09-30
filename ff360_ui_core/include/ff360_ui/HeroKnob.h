#pragma once

#include "DesignTokens.h"
#include "Fonts.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>

namespace ff360_ui {

class FF360_HeroKnob : public juce::Component, public juce::SettableTooltipClient {
public:
    FF360_HeroKnob(const juce::String& title, const juce::String& unit = "%")
        : m_title(title), m_unit(unit) {}

    void setValue(float normalizedVal, juce::NotificationType notify = juce::sendNotificationAsync) {
        normalizedVal = std::max(0.0f, std::min(1.0f, normalizedVal));
        if (std::abs(m_value - normalizedVal) > 1e-5f) {
            m_value = normalizedVal;
            repaint();
            if (notify != juce::dontSendNotification && onValueChanged) {
                onValueChanged(m_value);
            }
        }
    }

    float getValue() const noexcept { return m_value; }

    std::function<void(float)> onValueChanged;

    // Edit gesture around a drag or wheel step, for host automation recording and undo
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;

    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat();
        const float size = std::min(bounds.getWidth(), bounds.getHeight() - 28.0f);
        const float centreX = bounds.getCentreX();
        const float centreY = 14.0f + size * 0.5f;
        const float radius = size * 0.5f - 6.0f;

        const float startAngle = -juce::MathConstants<float>::pi * 0.75f;
        const float endAngle = juce::MathConstants<float>::pi * 0.75f;
        const float currentAngle = startAngle + m_value * (endAngle - startAngle);

        // Soft outer golden glow when engaged
        if (m_value > 0.01f) {
            g.setColour(juce::Colour(Colors::GoldSoft).withAlpha(0.18f * m_value));
            g.fillEllipse(centreX - radius - 6.0f, centreY - radius - 6.0f, (radius + 6.0f) * 2.0f, (radius + 6.0f) * 2.0f);
        }

        // Background track arc
        juce::Path trackPath;
        trackPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colour(Colors::TrackBackground));
        g.strokePath(trackPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active value arc
        if (m_value > 0.001f) {
            juce::Path valuePath;
            valuePath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, currentAngle, true);
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.strokePath(valuePath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner Dial Face
        const float innerR = radius - 8.0f;
        juce::ColourGradient grad(juce::Colour(0xFF333338), centreX - innerR * 0.3f, centreY - innerR * 0.3f,
                                  juce::Colour(0xFF131316), centreX, centreY, true);
        g.setGradientFill(grad);
        g.fillEllipse(centreX - innerR, centreY - innerR, innerR * 2.0f, innerR * 2.0f);

        // Gold inner rim border
        g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.4f));
        g.drawEllipse(centreX - innerR, centreY - innerR, innerR * 2.0f, innerR * 2.0f, 1.5f);

        // Center readout (0 to 100%)
        g.setColour(juce::Colour(Colors::TextOffWhite));
        g.setFont(ff360_ui::brandFont(15.0f, juce::Font::bold));
        const int pct = static_cast<int>(std::round(m_value * 100.0f));
        g.drawText(juce::String(pct) + m_unit, centreX - innerR, centreY - 10.0f, innerR * 2.0f, 20.0f, juce::Justification::centred);

        // Title Label at bottom
        g.setColour(juce::Colour(Colors::MetallicGold));
        g.setFont(ff360_ui::brandFont(11.0f, juce::Font::bold));
        g.drawText(m_title.toUpperCase(), 0, static_cast<int>(bounds.getBottom() - 20.0f), static_cast<int>(bounds.getWidth()), 18, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        m_dragStartVal = m_value;
        m_dragStartY = e.position.y;
        if (onDragStart) onDragStart();
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (onDragEnd) onDragEnd();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        const float deltaY = m_dragStartY - e.position.y;
        const float sensitivity = e.mods.isShiftDown() ? 0.001f : 0.005f;
        setValue(m_dragStartVal + deltaY * sensitivity);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override {
        if (onDragStart) onDragStart();
        setValue(m_value + wheel.deltaY * 0.05f);
        if (onDragEnd) onDragEnd();
    }

private:
    juce::String m_title;
    juce::String m_unit;
    float m_value = 0.0f;
    float m_dragStartVal = 0.0f;
    float m_dragStartY = 0.0f;
};

} // namespace ff360_ui

#endif
