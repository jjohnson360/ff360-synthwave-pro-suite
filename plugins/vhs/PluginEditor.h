#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class VHSPluginEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit VHSPluginEditor(VHSPluginProcessor&);
    ~VHSPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    VHSPluginProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    // Header Components
    juce::ComboBox m_presetBox;

    // Panels
    ff360_ui::FF360_GlassPanel m_macroPanel;
    ff360_ui::FF360_GlassPanel m_modulesPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    // Hero Macro Knob
    std::unique_ptr<ff360_ui::FF360_HeroKnob> m_degradeKnob;

    // 12 Module Sliders & Attachments
    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    // Master Knobs
    KnobControl m_inGainKnob;
    KnobControl m_outGainKnob;
    KnobControl m_mixKnob;

    // Meter
    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name, const juce::String& unit = "%");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VHSPluginEditor)
};

#endif
