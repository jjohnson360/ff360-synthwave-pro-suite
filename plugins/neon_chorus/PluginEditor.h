#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class NeonChorusEditor : public juce::AudioProcessorEditor {
public:
    explicit NeonChorusEditor(NeonChorusProcessor&);
    ~NeonChorusEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NeonChorusProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };

    ff360_ui::FF360_GlassPanel m_mainPanel;
    ff360_ui::PyramidScene m_scene;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    juce::TextButton m_quadButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_quadAttach;

    // Vintage/Modern character toggle (param exists in the DSP core but previously had no UI control)
    juce::TextButton m_vintageButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_vintageAttach;

    // Dots for cosmetic UI chrome
    struct DotIndicator : public juce::Component {
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colour(0x1FFFFFFF));
            g.fillEllipse(0, 0, 6, 6);
        }
    };
    DotIndicator m_d1, m_d2, m_d3;

    void createKnob(const std::string& id, const juce::String& name, bool useAmberAccent = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonChorusEditor)
};

#endif
