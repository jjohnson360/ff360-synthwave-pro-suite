#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class MidnightReverbEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit MidnightReverbEditor(MidnightReverbProcessor&);
    ~MidnightReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidnightReverbProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;
    juce::ComboBox m_algBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_algAttach;

    ff360_ui::FF360_GlassPanel m_decayPanel;
    ff360_ui::FF360_GlassPanel m_controlsPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    std::unique_ptr<ff360_ui::FF360_HeroKnob> m_decayHeroKnob;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    juce::ToggleButton m_syncToggle { "Tempo Sync" };
    juce::ToggleButton m_freezeToggle { "Infinite Freeze" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_syncAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_freezeAttach;

    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidnightReverbEditor)
};

#endif
