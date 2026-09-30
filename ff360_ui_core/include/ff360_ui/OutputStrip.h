#pragma once

#include "DesignTokens.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ff360/OutputStage.h"

namespace ff360_ui {

// Parameters for ff360::FF360_DSP_OutputStage, shared by every Synthwave plugin
namespace output {

inline constexpr const char* outGainId = "outGain";
inline constexpr const char* autoGainId = "autoGain";
inline constexpr const char* bypassId = "bypass";
inline constexpr const char* deltaId = "delta";

// withAutoGain: false for plugins where matching loudness would fight the effect itself
// (generators, tape stop). withOutGain: false when the plugin already has an "outGain" (VHS).
inline void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                          bool withAutoGain, bool withOutGain = true) {
    if (withOutGain)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ outGainId, 1 }, "Output", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    if (withAutoGain)
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ autoGainId, 1 }, "Auto Gain", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ bypassId, 1 }, "Bypass", false));
}

// Delta listen (ff360::FF360_DSP_DeltaTap): a listening aid, never stored in presets, undo or A/B
inline void addDeltaParameter(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) {
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ deltaId, 1 }, "Delta Listen", false));
}

inline bool readDelta(const juce::AudioProcessorValueTreeState& apvts) {
    auto* v = apvts.getRawParameterValue(deltaId);
    return v != nullptr && v->load() > 0.5f;
}

inline ff360::FF360_DSP_OutputStage::Settings readSettings(const juce::AudioProcessorValueTreeState& apvts) {
    ff360::FF360_DSP_OutputStage::Settings s;
    if (auto* v = apvts.getRawParameterValue(outGainId)) s.outputGainDb = v->load();
    if (auto* v = apvts.getRawParameterValue(autoGainId)) s.autoGain = v->load() > 0.5f;
    if (auto* v = apvts.getRawParameterValue(bypassId)) s.bypass = v->load() > 0.5f;
    s.holdAutoGain = readDelta(apvts);
    return s;
}

} // namespace output

// Footer row: [BYPASS] [Δ]  OUT ────●──── +0.0 dB  [OS 2x] [AUTO]
// AUTO shows the correction it's applying while on. Double-click the slider for 0 dB.
// Δ (delta listen), OS (oversampling) and AUTO only appear on plugins that have those parameters.
class OutputStrip : public juce::Component, private juce::Timer {
public:
    OutputStrip(juce::AudioProcessorValueTreeState& apvts, const ff360::FF360_DSP_OutputStage& stage)
        : m_stage(stage) {
        using Apvts = juce::AudioProcessorValueTreeState;

        m_bypass.setButtonText("BYPASS");
        m_bypass.setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::WarmAmberRed));
        m_bypass.setTooltip("Bypass the effect (10 ms crossfade to the input)");
        m_bypassAttach = std::make_unique<Apvts::ButtonAttachment>(apvts, output::bypassId, m_bypass);
        addAndMakeVisible(m_bypass);

        if (apvts.getParameter(output::deltaId) != nullptr) {
            m_delta.setButtonText(juce::String::fromUTF8("\xce\x94")); // Greek capital delta
            m_delta.setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::AccessibleSky));
            m_delta.setTooltip("Delta listen: hear only what the effect adds");
            m_deltaAttach = std::make_unique<Apvts::ButtonAttachment>(apvts, output::deltaId, m_delta);
            addAndMakeVisible(m_delta);
        }

        m_label.setText("OUT", juce::dontSendNotification);
        m_label.setFont(juce::Font(8.0f, juce::Font::bold));
        m_label.setBorderSize({});
        m_label.setColour(juce::Label::textColourId, juce::Colour(Colors::TextDim));
        addAndMakeVisible(m_label);

        m_gain.setSliderStyle(juce::Slider::LinearHorizontal);
        m_gain.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_gain.setTooltip("Output level. Double-click for 0 dB.");
        m_gainAttach = std::make_unique<Apvts::SliderAttachment>(apvts, output::outGainId, m_gain);
        m_gain.setDoubleClickReturnValue(true, 0.0);
        m_gain.onValueChange = [this] { updateGainText(); };
        m_gain.getProperties().set("ff360ShowsValue", true); // has its own dB readout
        addAndMakeVisible(m_gain);

        m_gainValue.setFont(juce::Font(9.0f, juce::Font::bold));
        m_gainValue.setJustificationType(juce::Justification::centredRight);
        m_gainValue.setColour(juce::Label::textColourId, juce::Colour(Colors::TextOffWhite));
        addAndMakeVisible(m_gainValue);
        updateGainText();

        if (apvts.getParameter(output::autoGainId) != nullptr) {
            m_auto.setButtonText("AUTO");
            m_auto.setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::MetallicGold));
            m_auto.setTooltip("Auto gain: match the output's loudness to the input, so you hear the effect, not the level change");
            m_autoAttach = std::make_unique<Apvts::ButtonAttachment>(apvts, output::autoGainId, m_auto);
            m_auto.onStateChange = [this] { updateAutoText(); };
            addAndMakeVisible(m_auto);
            startTimerHz(10);
        }

        if (auto* os = apvts.getParameter("oversampling")) {
            m_osParam = os;
            m_os.setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::MetallicGold));
            m_os.setClickingTogglesState(false); // opens a menu instead
            m_os.setTooltip("Oversampling: runs the effect at 2x or 4x the sample rate to reduce aliasing "
                            "(harsh digital artifacts from saturation and bit reduction). Adds a little latency and CPU.");
            m_os.onClick = [this] { showOversamplingMenu(); };
            m_osAttach = std::make_unique<juce::ParameterAttachment>(*os, [this](float index) { updateOversamplingText((int)index); });
            m_osAttach->sendInitialUpdate();
            addAndMakeVisible(m_os);
        }
    }

    void resized() override {
        auto b = getLocalBounds();
        m_bypass.setBounds(b.removeFromLeft(52));
        b.removeFromLeft(6);
        if (m_deltaAttach != nullptr) {
            m_delta.setBounds(b.removeFromLeft(26));
            b.removeFromLeft(6);
        }
        if (m_autoAttach != nullptr) {
            m_auto.setBounds(b.removeFromRight(48));
            b.removeFromRight(6);
        }
        if (m_osAttach != nullptr) {
            m_os.setBounds(b.removeFromRight(44));
            b.removeFromRight(6);
        }
        m_gainValue.setBounds(b.removeFromRight(42));
        // "OUT" caption above the slider, so the slider gets the full width of its column
        m_label.setBounds(b.removeFromTop(10));
        m_gain.setBounds(b);
    }

private:
    void updateGainText() {
        const double db = m_gain.getValue();
        m_gainValue.setText((db > 0.05 ? "+" : "") + juce::String(db, 1) + " dB", juce::dontSendNotification);
    }

    void updateAutoText() {
        if (!m_auto.getToggleState()) {
            m_auto.setButtonText("AUTO");
            return;
        }
        const float db = m_stage.getAppliedAutoGainDb();
        m_auto.setButtonText(juce::String(db > 0.05f ? "+" : "") + juce::String(db, 1));
    }

    void timerCallback() override { updateAutoText(); }

    void updateOversamplingText(int index) {
        static const char* names[] = { "OS off", "OS 2x", "OS 4x" };
        m_os.setButtonText(names[juce::jlimit(0, 2, index)]);
        m_os.setToggleState(index > 0, juce::dontSendNotification); // lit while oversampling
    }

    void showOversamplingMenu() {
        const int current = (int)m_osParam->convertFrom0to1(m_osParam->getValue());
        juce::PopupMenu menu;
        menu.setLookAndFeel(&getLookAndFeel());
        menu.addSectionHeader("Oversampling");
        menu.addItem(1, "Off", true, current == 0);
        menu.addItem(2, "2x", true, current == 1);
        menu.addItem(3, "4x", true, current == 2);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_os),
                           [safeThis = juce::Component::SafePointer<OutputStrip>(this)](int result) {
                               if (safeThis != nullptr && result > 0)
                                   safeThis->m_osAttach->setValueAsCompleteGesture((float)(result - 1));
                           });
    }

    const ff360::FF360_DSP_OutputStage& m_stage;

    juce::ToggleButton m_bypass, m_delta, m_auto, m_os; // drawn as pill toggles by FF360_LookAndFeel
    juce::RangedAudioParameter* m_osParam = nullptr;
    std::unique_ptr<juce::ParameterAttachment> m_osAttach;
    juce::Slider m_gain;
    juce::Label m_label, m_gainValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_bypassAttach, m_deltaAttach, m_autoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_gainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStrip)
};

} // namespace ff360_ui

#endif
