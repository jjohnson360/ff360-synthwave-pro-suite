#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class NightDriveEditor : public juce::AudioProcessorEditor {
public:
    explicit NightDriveEditor(NightDriveProcessor&);
    ~NightDriveEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NightDriveProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;

    ff360_ui::FF360_GlassPanel m_mainPanel;
    ff360_ui::VHSScene m_scene;

    struct FaderControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, FaderControl> m_faders;

    juce::Slider m_evolveSlider;
    juce::Label m_evolveLabel;
    juce::Label m_evolveValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_evolveAttach;

    juce::Slider m_mixSlider;
    juce::Label m_mixLabel;
    juce::Label m_mixValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mixAttach;

    juce::ComboBox m_scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_scaleAttach;

    // Sidechain duck amount (only audible once a host routes a sidechain signal in)
    juce::Slider m_scDuckSlider;
    juce::Label m_scDuckLabel;
    juce::Label m_scDuckValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_scDuckAttach;

    void createFader(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NightDriveEditor)
};

#endif
