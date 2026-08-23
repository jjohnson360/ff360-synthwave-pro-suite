#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class NeonWidthEditor : public juce::AudioProcessorEditor {
public:
    explicit NeonWidthEditor(NeonWidthProcessor&);
    ~NeonWidthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NeonWidthProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    ff360_ui::FF360_GlassPanel m_mainPanel;
    
    struct StereoScopeScene : public ff360_ui::BaseScene {
        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF1B0F1B), 0, 0,
                                        juce::Colour(0xFF09060A), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);
            
            g.setColour(juce::Colour(ff360_ui::Colors::WarmAmberRed).withAlpha(0.4f));
            float cx = bounds.getWidth() * 0.5f;
            float cy = bounds.getHeight() * 0.5f;
            
            g.drawEllipse(cx - 30, cy - 30, 60, 60, 1.0f);
            g.drawEllipse(cx - 20, cy - 20, 40, 40, 0.5f);
            g.drawLine(cx - 40, cy, cx + 40, cy, 0.5f);
            g.drawLine(cx, cy - 40, cx, cy + 40, 0.5f);
            
            g.setColour(juce::Colour(ff360_ui::Colors::WarmAmberRed));
            g.fillEllipse(cx + 10, cy - 10, 4, 4);
            g.fillEllipse(cx + 14, cy - 5, 3, 3);
            g.setColour(juce::Colour(ff360_ui::Colors::WarmAmberRed).withAlpha(0.4f));
            g.fillEllipse(cx - 10, cy + 8, 3, 3);
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
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, FaderControl> m_faders;

    juce::Slider m_bassMonoSlider;
    juce::Label m_bassMonoLabel;
    juce::Label m_bassMonoValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_bassMonoAttach;
    
    void createKnob(const std::string& id, const juce::String& name);
    void createFader(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonWidthEditor)
};

#endif
