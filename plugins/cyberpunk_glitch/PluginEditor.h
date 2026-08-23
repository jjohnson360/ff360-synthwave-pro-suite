#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class CyberpunkGlitchEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit CyberpunkGlitchEditor(CyberpunkGlitchProcessor&);
    ~CyberpunkGlitchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CyberpunkGlitchProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;
    juce::ComboBox m_divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_divisionAttach;

    ff360_ui::FF360_GlassPanel m_rhythmPanel;
    ff360_ui::FF360_GlassPanel m_dspPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    ff360_ui::FF360_HeroKnob m_probHeroKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_probAttach;

    juce::ToggleButton m_reverseToggle { "Reverse" };
    juce::ToggleButton m_freezeToggle { "Freeze" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_reverseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_freezeAttach;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CyberpunkGlitchEditor)
};

#endif
