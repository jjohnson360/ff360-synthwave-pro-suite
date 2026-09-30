#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/StereoFieldView.h"

class NeonWidthEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit NeonWidthEditor(NeonWidthProcessor&);
    ~NeonWidthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    NeonWidthProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };

    ff360_ui::FF360_GlassPanel m_mainPanel;

    // Live Lissajous goniometer + phase correlation meter, fed from the
    // processor's StereoEngine metrics and latest scope sample buffers.
    ff360_ui::FF360_StereoFieldView m_scope;

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
