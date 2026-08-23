#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"
#include "ff360_ui/StereoFieldView.h"

class NeonWidthEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit NeonWidthEditor(NeonWidthProcessor&);
    ~NeonWidthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeonWidthProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;

    ff360_ui::FF360_GlassPanel m_scopePanel;
    ff360_ui::FF360_GlassPanel m_imagingPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    ff360_ui::FF360_StereoFieldView m_stereoFieldView;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonWidthEditor)
};

#endif
