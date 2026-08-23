#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class NeonChorusEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit NeonChorusEditor(NeonChorusProcessor&);
    ~NeonChorusEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeonChorusProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;

    ff360_ui::FF360_GlassPanel m_chorusPanel;
    ff360_ui::FF360_GlassPanel m_modesPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    // Mode Toggles
    juce::ToggleButton m_vintageToggle { "Vintage Character" };
    juce::ToggleButton m_quadToggle { "Quad Chorus (4-Voice)" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_vintageAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_quadAttach;

    // Meter
    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonChorusEditor)
};

#endif
