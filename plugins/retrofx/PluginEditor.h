#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class RetroFXEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit RetroFXEditor(RetroFXProcessor&);
    ~RetroFXEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RetroFXProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;
    juce::ComboBox m_generatorBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_genAttach;

    juce::ComboBox m_syncBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_syncAttach;

    ff360_ui::FF360_GlassPanel m_heroPanel;
    ff360_ui::FF360_GlassPanel m_controlsPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    // Big Hero GENERATE Button/Knob
    ff360_ui::FF360_HeroKnob m_generateHeroKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_intensityAttach;

    juce::TextButton m_generateButton { "GENERATE" };

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroFXEditor)
};

#endif
