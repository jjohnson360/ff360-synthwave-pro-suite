#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class NightDriveEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit NightDriveEditor(NightDriveProcessor&);
    ~NightDriveEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NightDriveProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;
    juce::ComboBox m_scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_scaleAttach;

    ff360_ui::FF360_GlassPanel m_heroPanel;
    ff360_ui::FF360_GlassPanel m_layersPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    // Big Hero EVOLVE Knob
    ff360_ui::FF360_HeroKnob m_evolveHeroKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_evolveAttach;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    juce::Label m_chordFlowStatus;
    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NightDriveEditor)
};

#endif
