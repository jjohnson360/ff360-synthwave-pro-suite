#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/HeroKnob.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/MeterView.h"

class NeonTapeStopEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit NeonTapeStopEditor(NeonTapeStopProcessor&);
    ~NeonTapeStopEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeonTapeStopProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    juce::ComboBox m_presetBox;
    juce::ComboBox m_profileBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_profileAttach;

    ff360_ui::FF360_GlassPanel m_triggerPanel;
    ff360_ui::FF360_GlassPanel m_controlsPanel;
    ff360_ui::FF360_GlassPanel m_masterPanel;

    // Big illuminated momentary stop button
    juce::TextButton m_stopButton { "STOP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_stopAttach;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    juce::ToggleButton m_reverseToggle { "Reverse Recovery" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_reverseAttach;

    ff360_ui::FF360_MeterView m_meterView;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonTapeStopEditor)
};

#endif
