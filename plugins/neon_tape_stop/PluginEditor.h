#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

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
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };
    ff360_ui::OutputStrip m_outputStrip; // bypass, output trim, auto gain

    ff360_ui::FF360_GlassPanel m_mainPanel;
    
    struct ReelScene : public ff360_ui::BaseScene {
        float angle = 0.0f;
        // 1.0 = running at full normal speed, 0.0 = fully stopped (mirrors 1 - controller progress)
        float speedFactor = 1.0f;
        bool isStopped = false;

        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF131316), 0, 0,
                                        juce::Colour(0xFF0D0D10), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);

            // Hub/spoke colour drifts from gold (running) to amber-red (slowing/stopped)
            const auto reelColour = juce::Colour(ff360_ui::Colors::MetallicGold)
                                        .interpolatedWith(juce::Colour(ff360_ui::Colors::WarmAmberRed), 1.0f - speedFactor);

            auto drawReel = [&](float cx, float cy, float r) {
                g.setColour(juce::Colour(0xFF1A1A1E));
                g.fillEllipse(cx - r, cy - r, r*2, r*2);

                // Soft red glow ring while fully stopped
                if (isStopped) {
                    g.setColour(juce::Colour(ff360_ui::Colors::WarmAmberRed).withAlpha(0.35f));
                    g.drawEllipse(cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f, 2.0f);
                }

                g.setColour(reelColour);
                g.drawEllipse(cx - r, cy - r, r*2, r*2, 2.0f);

                juce::Path p;
                p.addEllipse(cx - 8, cy - 8, 16, 16);
                p.addLineSegment(juce::Line<float>(cx, cy - r + 4, cx, cy - 12), 2.0f);
                p.addLineSegment(juce::Line<float>(cx, cy + 12, cx, cy + r - 4), 2.0f);
                p.addLineSegment(juce::Line<float>(cx - r + 4, cy, cx - 12, cy), 2.0f);
                p.addLineSegment(juce::Line<float>(cx + 12, cy, cx + r - 4, cy), 2.0f);

                g.fillPath(p, juce::AffineTransform::rotation(angle, cx, cy));
            };

            drawReel(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.5f, 40.0f);
            drawReel(bounds.getWidth() * 0.7f, bounds.getHeight() * 0.5f, 40.0f);
        }
    } m_scene;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;
    
    juce::TextButton m_triggerBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_triggerAttach;

    juce::ToggleButton m_reverseToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_reverseAttach;

    juce::ComboBox m_profileBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_profileAttach;

    juce::Slider m_mixSlider;
    juce::Label m_mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_mixAttach;

    void createKnob(const std::string& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonTapeStopEditor)
};

#endif
