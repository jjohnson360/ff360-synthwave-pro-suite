#pragma once

#include "DesignTokens.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ff360_ui {

class FF360_LookAndFeel : public juce::LookAndFeel_V4 {
public:
    FF360_LookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Colors::DeepBlack));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Colors::MetallicGold));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Colors::TrackBackground));
        setColour(juce::Slider::thumbColourId, juce::Colour(Colors::MetallicGold));
        setColour(juce::Label::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::MatteCharcoal));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::GoldSoft));
        setColour(juce::ComboBox::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Colors::MatteCharcoal));
        setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::Charcoal2));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(Colors::MetallicGold));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override {
        const float radius = static_cast<float>(std::min(width, height)) * 0.5f - 4.0f;
        const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Track Arc
        juce::Path trackPath;
        trackPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(Colors::TrackBackground));
        g.strokePath(trackPath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value Arc
        if (sliderPosProportional > 0.001f) {
            juce::Path valuePath;
            valuePath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.strokePath(valuePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner Dial Face with radial gradient
        const float innerRadius = radius - 7.0f;
        juce::ColourGradient grad(juce::Colour(0xFF2B2B2F), centreX - innerRadius * 0.3f, centreY - innerRadius * 0.3f,
                                  juce::Colour(0xFF141416), centreX, centreY, true);
        g.setGradientFill(grad);
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

        // Thin hairline border
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

        // Pointer Needle
        juce::Path p;
        const float pointerLength = innerRadius * 0.65f;
        const float pointerThickness = 2.0f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius + 3.0f, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colour(Colors::MetallicGold));
        g.fillPath(p);
    }
};

} // namespace ff360_ui

#endif
