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
        setColour(juce::Slider::trackColourId, juce::Colour(Colors::TrackBackground));
        setColour(juce::Slider::backgroundColourId, juce::Colour(Colors::TrackBackground));
        
        setColour(juce::Label::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::MatteCharcoal));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::GoldSoft));
        setColour(juce::ComboBox::textColourId, juce::Colour(Colors::TextOffWhite));
        
        setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(Colors::GoldSoft));
        setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::MetallicGold));
        setColour(juce::TextButton::textColourOnId, juce::Colour(Colors::MetallicGold));
        
        setColour(juce::ToggleButton::textColourId, juce::Colour(Colors::TextDim));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::AccessibleSky));

        // Preset menu, tooltips and the Save / Delete preset dialogs
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Colors::MatteCharcoal));
        setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::PopupMenu::headerTextColourId, juce::Colour(Colors::MetallicGold));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::MetallicGold).withAlpha(0.18f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(Colors::TextOffWhite));

        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(Colors::DeepBlack));
        setColour(juce::TooltipWindow::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::TooltipWindow::outlineColourId, juce::Colour(Colors::MetallicGold).withAlpha(0.4f));

        setColour(juce::AlertWindow::backgroundColourId, juce::Colour(Colors::MatteCharcoal));
        setColour(juce::AlertWindow::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::AlertWindow::outlineColourId, juce::Colour(Colors::MetallicGold).withAlpha(0.4f));
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(Colors::DeepBlack));
        setColour(juce::TextEditor::textColourId, juce::Colour(Colors::TextOffWhite));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(Colors::MetallicGold).withAlpha(0.3f));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(Colors::MetallicGold));
        setColour(juce::TextEditor::highlightColourId, juce::Colour(Colors::MetallicGold).withAlpha(0.3f));
        setColour(juce::CaretComponent::caretColourId, juce::Colour(Colors::MetallicGold));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override {
        const float radius = static_cast<float>(std::min(width, height)) * 0.5f - 4.0f;
        const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Check if amber accent requested (hack using slider thumb colour property)
        juce::Colour accentColour = slider.findColour(juce::Slider::thumbColourId, true);
        if (accentColour == juce::Colours::transparentWhite) accentColour = juce::Colour(Colors::MetallicGold);

        juce::Path trackPath;
        trackPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(Colors::TrackBackground));
        g.strokePath(trackPath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPosProportional > 0.001f) {
            juce::Path valuePath;
            valuePath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(accentColour);
            g.strokePath(valuePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float innerRadius = radius - 7.0f;
        juce::ColourGradient grad(juce::Colour(0xFF2B2B2F), centreX - innerRadius * 0.3f, centreY - innerRadius * 0.3f,
                                  juce::Colour(0xFF141416), centreX, centreY, true);
        g.setGradientFill(grad);
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

        juce::Path p;
        const float pointerLength = innerRadius * 0.65f;
        const float pointerThickness = 2.0f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius + 3.0f, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(accentColour);
        g.fillPath(p);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override {
        
        juce::Colour accentColour = slider.findColour(juce::Slider::thumbColourId, true);
        
        if (slider.isHorizontal()) {
            float trackHeight = 5.0f;
            float cy = y + height * 0.5f;
            
            // Track
            g.setColour(juce::Colour(Colors::TrackBackground));
            g.fillRoundedRectangle(x, cy - trackHeight * 0.5f, width, trackHeight, 2.0f);
            
            // Fill
            if (sliderPos > x) {
                juce::ColourGradient grad(accentColour, x, cy, juce::Colour(0xFFE8C98A), sliderPos, cy, false);
                g.setGradientFill(grad);
                g.fillRoundedRectangle(x, cy - trackHeight * 0.5f, sliderPos - x, trackHeight, 2.0f);
            }
            
            // Handle
            g.setColour(accentColour);
            g.fillEllipse(sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f);
            g.setColour(juce::Colour(Colors::DeepBlack));
            g.drawEllipse(sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f, 2.0f);
            
        } else {
            float trackWidth = 5.0f;
            float cx = x + width * 0.5f;
            
            // Track
            g.setColour(juce::Colour(Colors::TrackBackground));
            g.fillRoundedRectangle(cx - trackWidth * 0.5f, y, trackWidth, height, 2.0f);
            
            // Fill (Bottom up)
            float fillHeight = y + height - sliderPos;
            if (fillHeight > 0) {
                juce::ColourGradient grad(juce::Colour(0xFFE8C98A), cx, sliderPos, accentColour, cx, y + height, false);
                g.setGradientFill(grad);
                g.fillRoundedRectangle(cx - trackWidth * 0.5f, sliderPos, trackWidth, fillHeight, 2.0f);
            }
            
            // Handle
            g.setColour(accentColour);
            g.fillRoundedRectangle(cx - 7.0f, sliderPos - 4.0f, 14.0f, 8.0f, 2.0f);
            g.setColour(juce::Colour(Colors::DeepBlack));
            g.drawRoundedRectangle(cx - 7.0f, sliderPos - 4.0f, 14.0f, 8.0f, 2.0f, 1.0f);
        }
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        bounds.reduce(0.5f, 0.5f);
        float radius = 8.0f;
        
        if (shouldDrawButtonAsDown || button.getToggleState()) {
            g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.1f));
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.drawRoundedRectangle(bounds, radius, 1.5f);
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.05f));
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        } else {
            g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.7f));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        }
    }
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, 
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        bounds.reduce(0.5f, 0.5f);
        float radius = 6.0f;
        bool isToggled = button.getToggleState();
        
        juce::Colour accentColour = button.findColour(juce::ToggleButton::tickColourId);
        
        if (isToggled) {
            g.setColour(accentColour.withAlpha(0.08f));
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(accentColour);
            g.drawRoundedRectangle(bounds, radius, 1.0f);
            
            // subtle glow
            g.setColour(accentColour.withAlpha(0.15f));
            g.drawRoundedRectangle(bounds.expanded(1.0f), radius+1.0f, 2.0f);
        } else {
            g.setColour(juce::Colour(0x07FFFFFF));
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(juce::Colour(0x24FFFFFF));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
            
            if (shouldDrawButtonAsHighlighted) {
                g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.5f));
                g.drawRoundedRectangle(bounds, radius, 1.0f);
            }
        }
        
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.setColour(isToggled ? accentColour : juce::Colour(Colors::TextDim));
        
        // Custom text layout
        juce::String text = button.getButtonText().toUpperCase();
        g.drawText(text, bounds.toNearestInt(), juce::Justification::centred, false);
    }
};

} // namespace ff360_ui
#endif
