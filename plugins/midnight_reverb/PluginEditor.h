#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/ControlHelpers.h"
#include "ff360_ui/EditorScaling.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"
#include "ff360_ui/HeroKnob.h"

class MidnightReverbEditor : public juce::AudioProcessorEditor {
public:
    explicit MidnightReverbEditor(MidnightReverbProcessor&);
    ~MidnightReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MidnightReverbProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };
    ff360_ui::OutputStrip m_outputStrip; // bypass, output trim, auto gain

    ff360_ui::FF360_GlassPanel m_mainPanel;
    // We will use a generic scene or add TunnelScene later, for now BaseScene with custom paint
    
    struct TunnelScene : public ff360_ui::BaseScene {
        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF0F1B29), 0, 0,
                                        juce::Colour(0xFF05080E), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);
            
            g.setColour(juce::Colour(ff360_ui::Colors::AccessibleSky).withAlpha(0.3f));
            float cx = bounds.getWidth() * 0.5f;
            float cy = bounds.getHeight() * 0.5f;
            for(int i = 0; i < 6; ++i) {
                float size = 20.0f + i * 30.0f;
                g.drawRect(cx - size*0.5f, cy - size*0.5f, size, size, 1.0f);
                g.drawLine(cx - size*0.5f, cy - size*0.5f, cx - (size-30)*0.5f, cy - (size-30)*0.5f);
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
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, FaderControl> m_faders;

    juce::ComboBox m_algBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_algAttach;

    juce::TextButton m_freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_freezeAttach;

    // Tempo Sync toggle for pre-delay (param exists in the DSP core but previously had no UI control)
    juce::TextButton m_syncButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_syncAttach;

    void createKnob(const std::string& id, const juce::String& name);
    void createFader(const std::string& id, const juce::String& name, const juce::Colour& accentColour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidnightReverbEditor)
};

#endif
