#pragma once

#include "DesignTokens.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>

namespace ff360_ui {

class BaseScene : public juce::Component {
public:
    BaseScene() {}
    virtual void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) = 0;
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xFF0D0D10));
        g.fillRoundedRectangle(bounds, 8.0f);
        paintScene(g, bounds);
    }
};

class VHSScene : public BaseScene {
public:
    void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
        // Gradient background
        juce::ColourGradient bgGrad(juce::Colour(0xFF3A2B52), 0, 0,
                                    juce::Colour(0xFF0D0B0E), 0, bounds.getHeight(), false);
        bgGrad.addColour(0.38, juce::Colour(0xFF8A4A55));
        bgGrad.addColour(0.58, juce::Colour(0xFFE8935A));
        bgGrad.addColour(0.68, juce::Colour(0xFFF5B56A));
        bgGrad.addColour(0.68, juce::Colour(0xFF1A1418));
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, 8.0f);
        
        float w = bounds.getWidth();
        float h = bounds.getHeight();
        
        // Sun
        float sunR = h > 130 ? 33.0f : 29.0f;
        float sunX = w * 0.5f;
        float sunY = h * 0.44f;
        juce::ColourGradient sunGrad(juce::Colour(0xFFFFE6B0), sunX - sunR*0.2f, sunY - sunR*0.3f,
                                     juce::Colour(0xFFE8654A), sunX, sunY + sunR, true);
        sunGrad.addColour(0.55, juce::Colour(0xFFF2B25A));
        g.setGradientFill(sunGrad);
        g.fillEllipse(sunX - sunR, sunY - sunR, sunR*2, sunR*2);
        
        // Horizon
        float horizonY = h * 0.68f;
        g.setColour(juce::Colour(0x99F5B56A));
        g.drawLine(0, horizonY, w, horizonY, 1.0f);
        
        // Grid lines (simplified)
        g.setColour(juce::Colour(0x22E8C48A));
        for (int i = 0; i < 10; ++i) {
            float y = horizonY + std::pow(i, 1.5f) * 2.0f;
            if (y > h) break;
            g.drawLine(0, y, w, y, 1.0f);
        }
        for (int i = -10; i <= 10; ++i) {
            float x1 = w * 0.5f + i * 20.0f;
            float x2 = w * 0.5f + i * 60.0f;
            g.drawLine(x1, horizonY, x2, h, 1.0f);
        }
    }
};

class PyramidScene : public BaseScene {
public:
    void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
        float cx = bounds.getWidth() * 0.5f;
        float cy = bounds.getHeight() * 0.5f;
        
        g.setColour(juce::Colour(Colors::AccessibleSky));
        juce::Path p1;
        p1.addTriangle(cx, cy - 30, cx + 40, cy + 30, cx - 40, cy + 30);
        g.strokePath(p1, juce::PathStrokeType(2.0f));
        
        g.setColour(juce::Colour(Colors::AccessibleSky).withAlpha(0.6f));
        juce::Path p2;
        p2.addTriangle(cx, cy - 12, cx + 24, cy + 30, cx - 24, cy + 30);
        g.strokePath(p2, juce::PathStrokeType(1.4f));
        
        g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.7f));
        juce::Path p3;
        p3.addTriangle(cx, cy + 2, cx + 14, cy + 30, cx - 14, cy + 30);
        g.strokePath(p3, juce::PathStrokeType(1.2f));
        
        g.setFont(juce::Font(15.0f, juce::Font::bold).withStyle(juce::Font::italic));
        g.setColour(juce::Colour(Colors::AccessibleSky));
        g.drawText("Neon Chorus", bounds.withTrimmedTop(8).withHeight(20), juce::Justification::centredTop);
    }
};

// We will add the other scenes as we implement their respective plugins,
// or we can just keep them empty shells that draw their titles.

} // namespace ff360_ui

#endif
