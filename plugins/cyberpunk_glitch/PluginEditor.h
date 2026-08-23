#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class CyberpunkGlitchEditor : public juce::AudioProcessorEditor {
public:
    explicit CyberpunkGlitchEditor(CyberpunkGlitchProcessor&);
    ~CyberpunkGlitchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CyberpunkGlitchProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    ff360_ui::FF360_GlassPanel m_mainPanel;
    
    struct GlitchGridScene : public ff360_ui::BaseScene {
        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF0F1B29), 0, 0,
                                        juce::Colour(0xFF05080E), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);
            
            float cw = bounds.getWidth() / 16.0f;
            float heights[16] = {0.6f, 0.35f, 0.8f, 0.5f, 0.7f, 0.2f, 0.9f, 0.45f,
                                 0.3f, 0.65f, 0.55f, 0.4f, 0.75f, 0.25f, 0.6f, 0.48f};
            
            juce::Colour cSky(ff360_ui::Colors::AccessibleSky);
            juce::Colour cAmber(ff360_ui::Colors::WarmAmberRed);
            juce::Colour cGold(ff360_ui::Colors::MetallicGold);
            
            juce::Colour cols[16] = {cSky, cSky, cAmber, cAmber, cGold, cSky, cAmber, cSky,
                                     cGold, cAmber, cSky, cGold, cAmber, cSky, cGold, cAmber};
            
            for(int i = 0; i < 16; ++i) {
                float x = i * cw;
                float h = bounds.getHeight() * heights[i];
                float y = (bounds.getHeight() - h) * 0.5f;
                g.setColour(cols[i]);
                g.fillRect(x + 2, y, cw - 4, h);
            }
        }
    } m_scene;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;
    
    struct FaderControl {
        juce::Slider slider;
        juce::Label label;
        juce::Label valueLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, FaderControl> m_faders;

    juce::ComboBox m_divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_divisionAttach;

    juce::ToggleButton m_freezeToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_freezeAttach;

    juce::ToggleButton m_reverseToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_reverseAttach;

    void createKnob(const std::string& id, const juce::String& name, bool amber = false);
    void createHFader(const std::string& id, const juce::String& name, bool amber = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CyberpunkGlitchEditor)
};

#endif
