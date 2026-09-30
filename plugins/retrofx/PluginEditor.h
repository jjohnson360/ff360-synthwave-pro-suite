#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/ControlHelpers.h"
#include "ff360_ui/EditorScaling.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class RetroFXEditor : public juce::AudioProcessorEditor {
public:
    explicit RetroFXEditor(RetroFXProcessor&);
    ~RetroFXEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    RetroFXProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };
    ff360_ui::OutputStrip m_outputStrip; // bypass, output trim, auto gain

    ff360_ui::FF360_GlassPanel m_mainPanel;
    
    struct WaveformScene : public ff360_ui::BaseScene {
        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF0F1B29), 0, 0,
                                        juce::Colour(0xFF05080E), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);
            
            g.setColour(juce::Colour(ff360_ui::Colors::AccessibleSky));
            juce::Path wave;
            wave.startNewSubPath(0, bounds.getHeight() * 0.5f);
            for (float x = 0; x < bounds.getWidth(); x += 5.0f) {
                float y = std::sin(x * 0.1f) * 20.0f * (1.0f - x/bounds.getWidth());
                wave.lineTo(x, bounds.getHeight() * 0.5f + y);
            }
            g.strokePath(wave, juce::PathStrokeType(2.0f));
        }
    } m_scene;

    juce::ListBox m_typeList; // Or we can just use a combobox or a set of buttons, but we'll use a ComboBox for simplicity if ListBox is too much code
    juce::ComboBox m_generatorBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_genAttach;

    juce::ComboBox m_syncBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_syncAttach;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;

    juce::Slider m_mixSlider;
    juce::Label m_mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mixAttach;

    juce::TextButton m_generateBtn;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroFXEditor)
};

#endif
