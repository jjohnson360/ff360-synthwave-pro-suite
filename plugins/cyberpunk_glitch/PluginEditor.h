#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "ff360_ui/LookAndFeel.h"
#include "ff360_ui/WorkflowBar.h"
#include "ff360_ui/ControlHelpers.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360_ui/GlassPanel.h"
#include "ff360_ui/Scenes.h"

class CyberpunkGlitchEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit CyberpunkGlitchEditor(CyberpunkGlitchProcessor&);
    ~CyberpunkGlitchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CyberpunkGlitchProcessor& m_processor;
    ff360_ui::FF360_LookAndFeel m_lookAndFeel;
    ff360_ui::WorkflowBar m_workflowBar; // undo/redo, presets, A/B
    juce::TooltipWindow m_tooltips { this, 600 };
    ff360_ui::OutputStrip m_outputStrip; // bypass, output trim, auto gain

    ff360_ui::FF360_GlassPanel m_mainPanel;

    // Live step-grid visualizer: each cell reflects a real recent grid-step trigger
    // decision from GlitchEngine, and the current cell pulses while a slice is
    // actually being played back (not a hard-coded decorative pattern).
    struct GlitchGridScene : public ff360_ui::BaseScene {
        std::array<bool, ff360::FF360_DSP_GlitchEngine::kStepHistorySize> stepHistory{};
        bool isLive = false;
        float gridProgress = 0.0f;

        void paintScene(juce::Graphics& g, juce::Rectangle<float> bounds) override {
            juce::ColourGradient bgGrad(juce::Colour(0xFF0F1B29), 0, 0,
                                        juce::Colour(0xFF05080E), 0, bounds.getHeight(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 8.0f);

            const size_t numSteps = stepHistory.size();
            const float cw = bounds.getWidth() / static_cast<float>(numSteps);

            juce::Colour cSky(ff360_ui::Colors::AccessibleSky);
            juce::Colour cAmber(ff360_ui::Colors::WarmAmberRed);
            juce::Colour cGold(ff360_ui::Colors::MetallicGold);
            juce::Colour palette[3] = { cSky, cAmber, cGold };

            for (size_t i = 0; i < numSteps; ++i) {
                const bool hit = stepHistory[i];
                const bool isCurrent = (i == numSteps - 1);
                const float x = static_cast<float>(i) * cw;

                // Hit steps stand tall and lit in a rotating accent colour; misses sit low and dim.
                const float h = bounds.getHeight() * (hit ? (0.45f + 0.4f * ((i * 37) % 100) / 100.0f) : 0.12f);
                const float y = bounds.getHeight() - h;

                auto colour = hit ? palette[i % 3].withAlpha(0.85f)
                                   : juce::Colour(ff360_ui::Colors::TrackBackground);

                // Pulse the most recent step while a slice is actively playing back.
                if (isCurrent && isLive) {
                    colour = colour.brighter(0.4f);
                }

                g.setColour(colour);
                g.fillRect(x + 2.0f, y, cw - 4.0f, h);

                if (isCurrent) {
                    g.setColour(juce::Colour(ff360_ui::Colors::TextOffWhite).withAlpha(0.5f));
                    g.drawRect(x + 1.0f, 1.0f, cw - 2.0f, bounds.getHeight() - 2.0f, 1.0f);
                }
            }

            // Playhead sweep across the current grid subdivision
            const float phX = bounds.getWidth() - cw + cw * gridProgress;
            g.setColour(juce::Colour(ff360_ui::Colors::TextOffWhite).withAlpha(0.3f));
            g.drawVerticalLine(static_cast<int>(phX), 0.0f, bounds.getHeight());
        }
    } m_scene;

    struct KnobControl {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, KnobControl> m_knobs;
    
    struct FaderControl {
        juce::Slider slider;
        juce::Label label;
        juce::Label valueLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unordered_map<std::string, FaderControl> m_faders;

    juce::ComboBox m_divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_divisionAttach;

    juce::ToggleButton m_freezeToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_freezeAttach;

    juce::ToggleButton m_reverseToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_reverseAttach;

    void createKnob(const std::string& id, const juce::String& name, bool amber = false);
    void createHFader(const std::string& id, const juce::String& name, bool amber = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CyberpunkGlitchEditor)
};

#endif
