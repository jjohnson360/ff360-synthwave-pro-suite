#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

CyberpunkGlitchEditor::CyberpunkGlitchEditor(CyberpunkGlitchProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_rhythmPanel("RHYTHM & PROBABILITY"),
      m_dspPanel("GLITCH PROCESSORS"),
      m_masterPanel("OUTPUT / METERING"),
      m_probHeroKnob("PROBABILITY", "%") {
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

    addAndMakeVisible(m_rhythmPanel);
    addAndMakeVisible(m_dspPanel);
    addAndMakeVisible(m_masterPanel);

    // Division Box
    static const juce::StringArray divNames = { "1/4 Note", "1/8 Note", "1/16 Note", "1/32 Note" };
    for (int i = 0; i < divNames.size(); ++i) {
        m_divisionBox.addItem(divNames[i], i + 1);
    }
    m_divisionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "division", m_divisionBox);
    m_rhythmPanel.addAndMakeVisible(m_divisionBox);

    // Hero Probability Knob
    m_probAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "probability", m_probHeroKnob.getSlider());
    m_rhythmPanel.addAndMakeVisible(m_probHeroKnob);

    // Toggles
    m_reverseToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_reverseToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "reverse", m_reverseToggle);
    m_rhythmPanel.addAndMakeVisible(m_reverseToggle);

    m_freezeToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_freezeToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    m_freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "freeze", m_freezeToggle);
    m_rhythmPanel.addAndMakeVisible(m_freezeToggle);

    // DSP Knobs
    createKnob("pitch", "PITCH");
    createKnob("bitcrush", "BITCRUSH");
    createKnob("gate", "GATE");
    createKnob("filter", "FILTER");
    createKnob("resonance", "RESONANCE");
    createKnob("mix", "MIX");

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(920, 520);
    startTimerHz(30);
}

CyberpunkGlitchEditor::~CyberpunkGlitchEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void CyberpunkGlitchEditor::createKnob(const std::string& id, const juce::String& name) {
    auto& kc = m_knobs[id];
    kc.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    kc.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    kc.label.setText(name, juce::dontSendNotification);
    kc.label.setJustificationType(juce::Justification::centred);
    kc.label.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    kc.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    kc.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, kc.slider);

    if (id == "mix") {
        m_masterPanel.addAndMakeVisible(kc.slider);
        m_masterPanel.addAndMakeVisible(kc.label);
    } else {
        m_dspPanel.addAndMakeVisible(kc.slider);
        m_dspPanel.addAndMakeVisible(kc.label);
    }
}

void CyberpunkGlitchEditor::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::DeepBlack), 0, 0,
                                juce::Colour(0xFF070708), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.setFont(juce::Font(juce::FontOptions().withHeight(20.0f).withStyle("Bold")));
    g.drawText("ff360_labs", 24, 16, 120, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextOffWhite));
    g.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold Italic")));
    g.drawText("Cyberpunk Glitch", 140, 17, 180, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("TEMPO-SYNCED STUTTER & BUFFER GLITCH", 325, 21, 280, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void CyberpunkGlitchEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int rhythmW = 260;
    const int masterW = 150;
    const int dspW = getWidth() - rhythmW - masterW - (pad * 4);

    m_rhythmPanel.setBounds(pad, top, rhythmW, contentH);
    m_dspPanel.setBounds(pad * 2 + rhythmW, top, dspW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Rhythm Panel
    m_divisionBox.setBounds(20, 35, rhythmW - 40, 26);
    m_probHeroKnob.setBounds((rhythmW - 140) / 2, 75, 140, 150);
    m_reverseToggle.setBounds(25, contentH - 65, 100, 24);
    m_freezeToggle.setBounds(135, contentH - 65, 100, 24);

    // DSP Panel: 3 cols x 2 rows
    static const std::vector<std::string> dspList = { "pitch", "bitcrush", "gate", "filter", "resonance" };
    const int cols = 3;
    const int rows = 2;
    const int cellW = (dspW - 20) / cols;
    const int cellH = (contentH - 45) / rows;

    for (size_t i = 0; i < dspList.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 35 + row * cellH;

        auto& kc = m_knobs[dspList[i]];
        const int knobSize = std::min(cellW - 10, cellH - 24);
        kc.slider.setBounds(x + (cellW - knobSize) / 2, y, knobSize, knobSize);
        kc.label.setBounds(x, y + knobSize + 2, cellW, 14);
    }

    // Master Panel
    auto& mixKnob = m_knobs["mix"];
    mixKnob.slider.setBounds((masterW - 64) / 2, 40, 64, 64);
    mixKnob.label.setBounds(10, 108, masterW - 20, 14);

    m_meterView.setBounds(25, 145, masterW - 50, contentH - 165);
}

void CyberpunkGlitchEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
