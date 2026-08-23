#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

NeonChorusEditor::NeonChorusEditor(NeonChorusProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_chorusPanel("CHORUS ENGINE"),
      m_modesPanel("CONFIGURATION / VOICING"),
      m_masterPanel("OUTPUT / METERING") {
    setLookAndFeel(&m_lookAndFeel);

    // Preset selector
    m_presetBox.setTextWhenNothingSelected("Select Preset...");
    for (int i = 0; i < m_processor.getNumPrograms(); ++i) {
        m_presetBox.addItem(m_processor.getProgramName(i), i + 1);
    }
    m_presetBox.setSelectedId(m_processor.getCurrentProgram() + 1, juce::dontSendNotification);
    m_presetBox.onChange = [this]() {
        m_processor.setCurrentProgram(m_presetBox.getSelectedId() - 1);
    };
    addAndMakeVisible(m_presetBox);

    addAndMakeVisible(m_chorusPanel);
    addAndMakeVisible(m_modesPanel);
    addAndMakeVisible(m_masterPanel);

    // Core Knobs
    createKnob("rate", "RATE");
    createKnob("depth", "DEPTH");
    createKnob("width", "WIDTH");
    createKnob("detune", "DETUNE");
    createKnob("feedback", "FEEDBACK");
    createKnob("predelay", "PRE-DELAY");
    createKnob("bassmono", "BASS MONO");
    createKnob("mix", "MIX");

    // Mode Toggles
    m_vintageToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_vintageToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_vintageAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "vintage", m_vintageToggle);
    m_modesPanel.addAndMakeVisible(m_vintageToggle);

    m_quadToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_quadToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_quadAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "quad", m_quadToggle);
    m_modesPanel.addAndMakeVisible(m_quadToggle);

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(800, 480);
    startTimerHz(30);
}

NeonChorusEditor::~NeonChorusEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NeonChorusEditor::createKnob(const std::string& id, const juce::String& name) {
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
        m_chorusPanel.addAndMakeVisible(kc.slider);
        m_chorusPanel.addAndMakeVisible(kc.label);
    }
}

void NeonChorusEditor::paint(juce::Graphics& g) {
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
    g.drawText("Neon Chorus", 140, 17, 130, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("80s STEREO & QUAD PAD CHORUS", 276, 21, 240, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void NeonChorusEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int masterW = 150;
    const int modesW = 200;
    const int chorusW = getWidth() - masterW - modesW - (pad * 4);

    m_chorusPanel.setBounds(pad, top, chorusW, contentH);
    m_modesPanel.setBounds(pad * 2 + chorusW, top, modesW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Chorus Knobs: 3 columns x 2 rows
    static const std::vector<std::string> mainKnobs = { "rate", "depth", "width", "detune", "feedback", "predelay" };
    const int cols = 3;
    const int rows = 2;
    const int cellW = (chorusW - 20) / cols;
    const int cellH = (contentH - 45) / rows;

    for (size_t i = 0; i < mainKnobs.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 40 + row * cellH;

        auto& kc = m_knobs[mainKnobs[i]];
        const int knobSize = std::min(cellW - 10, cellH - 24);
        kc.slider.setBounds(x + (cellW - knobSize) / 2, y, knobSize, knobSize);
        kc.label.setBounds(x, y + knobSize + 2, cellW, 14);
    }

    // Modes Panel
    m_vintageToggle.setBounds(20, 45, modesW - 40, 28);
    m_quadToggle.setBounds(20, 85, modesW - 40, 28);

    auto& bassMonoKnob = m_knobs["bassmono"];
    bassMonoKnob.slider.setBounds((modesW - 70) / 2, 140, 70, 70);
    bassMonoKnob.label.setBounds(10, 215, modesW - 20, 14);

    // Master Panel
    auto& mixKnob = m_knobs["mix"];
    mixKnob.slider.setBounds((masterW - 64) / 2, 40, 64, 64);
    mixKnob.label.setBounds(10, 108, masterW - 20, 14);

    m_meterView.setBounds(25, 145, masterW - 50, contentH - 165);
}

void NeonChorusEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
