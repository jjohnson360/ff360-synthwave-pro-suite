#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

NeonTapeStopEditor::NeonTapeStopEditor(NeonTapeStopProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_triggerPanel("TRIGGER & PROFILE"),
      m_controlsPanel("SLOWDOWN DYNAMICS"),
      m_masterPanel("OUTPUT / METERING") {
    setLookAndFeel(&m_lookAndFeel);

    m_presetBox.setTextWhenNothingSelected("Select Preset...");
    for (int i = 0; i < m_processor.getNumPrograms(); ++i) {
        m_presetBox.addItem(m_processor.getProgramName(i), i + 1);
    }
    m_presetBox.setSelectedId(m_processor.getCurrentProgram() + 1, juce::dontSendNotification);
    m_presetBox.onChange = [this]() {
        m_processor.setCurrentProgram(m_presetBox.getSelectedId() - 1);
    };
    addAndMakeVisible(m_presetBox);

    addAndMakeVisible(m_triggerPanel);
    addAndMakeVisible(m_controlsPanel);
    addAndMakeVisible(m_masterPanel);

    // Stop Profile selector
    static const juce::StringArray profileNames = { "Vinyl Stop", "Tape Stop", "Digital Stop" };
    for (int i = 0; i < profileNames.size(); ++i) {
        m_profileBox.addItem(profileNames[i], i + 1);
    }
    m_profileAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "profile", m_profileBox);
    m_triggerPanel.addAndMakeVisible(m_profileBox);

    // Big illuminated stop button
    m_stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    m_stopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_stopButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF000000));
    m_stopButton.setColour(juce::TextButton::textColourOffId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_stopAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "trigger", m_stopButton);
    m_triggerPanel.addAndMakeVisible(m_stopButton);

    // Controls
    createKnob("slowdown", "SLOWDOWN");
    createKnob("pitchcurve", "PITCH CURVE");
    createKnob("filtermove", "FILTER SWEEP");
    createKnob("recovery", "RECOVERY");
    createKnob("mix", "MIX");

    m_reverseToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_reverseToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "reverse", m_reverseToggle);
    m_controlsPanel.addAndMakeVisible(m_reverseToggle);

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(840, 480);
    startTimerHz(30);
}

NeonTapeStopEditor::~NeonTapeStopEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NeonTapeStopEditor::createKnob(const std::string& id, const juce::String& name) {
    auto& kc = m_knobs[id];
    kc.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    kc.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    kc.label.setText(name, juce::dontSendNotification);
    kc.label.setJustificationType(juce::Justification::centred);
    kc.label.setFont(juce::Font(10.0f, juce::Font::plain));
    kc.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    kc.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, kc.slider);

    if (id == "mix") {
        m_masterPanel.addAndMakeVisible(kc.slider);
        m_masterPanel.addAndMakeVisible(kc.label);
    } else {
        m_controlsPanel.addAndMakeVisible(kc.slider);
        m_controlsPanel.addAndMakeVisible(kc.label);
    }
}

void NeonTapeStopEditor::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::DeepBlack), 0, 0,
                                juce::Colour(0xFF070708), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("ff360_labs", 24, 16, 120, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextOffWhite));
    g.setFont(juce::Font(18.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("Neon Tape Stop", 140, 17, 160, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("ANALOG / VINYL / DIGITAL TAPE STOP", 305, 21, 260, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void NeonTapeStopEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int triggerW = 220;
    const int masterW = 150;
    const int controlsW = getWidth() - triggerW - masterW - (pad * 4);

    m_triggerPanel.setBounds(pad, top, triggerW, contentH);
    m_controlsPanel.setBounds(pad * 2 + triggerW, top, controlsW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Trigger Panel
    m_profileBox.setBounds(20, 40, triggerW - 40, 26);
    m_stopButton.setBounds(30, 95, triggerW - 60, triggerW - 60);

    // Controls: 2 cols x 2 rows
    static const std::vector<std::string> controlList = { "slowdown", "pitchcurve", "filtermove", "recovery" };
    const int cols = 2;
    const int rows = 2;
    const int cellW = (controlsW - 20) / cols;
    const int cellH = (contentH - 60) / rows;

    for (size_t i = 0; i < controlList.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 35 + row * cellH;

        auto& kc = m_knobs[controlList[i]];
        const int knobSize = std::min(cellW - 10, cellH - 24);
        kc.slider.setBounds(x + (cellW - knobSize) / 2, y, knobSize, knobSize);
        kc.label.setBounds(x, y + knobSize + 2, cellW, 14);
    }

    m_reverseToggle.setBounds(15, contentH - 35, 160, 24);

    // Master Panel
    auto& mixKnob = m_knobs["mix"];
    mixKnob.slider.setBounds((masterW - 64) / 2, 40, 64, 64);
    mixKnob.label.setBounds(10, 108, masterW - 20, 14);

    m_meterView.setBounds(25, 145, masterW - 50, contentH - 165);
}

void NeonTapeStopEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
