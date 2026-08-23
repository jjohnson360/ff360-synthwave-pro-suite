#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

VHSPluginEditor::VHSPluginEditor(VHSPluginProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_macroPanel("DEGRADE MACRO"),
      m_modulesPanel("TAPE DEGRADATION MODULES"),
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

    // Panels
    addAndMakeVisible(m_macroPanel);
    addAndMakeVisible(m_modulesPanel);
    addAndMakeVisible(m_masterPanel);

    // Hero Macro Knob
    m_degradeKnob = std::make_unique<ff360_ui::FF360_HeroKnob>("DEGRADE", "%");
    if (auto* param = m_processor.getApvts().getRawParameterValue("degrade")) {
        m_degradeKnob->setValue(param->load() * 0.01f, juce::dontSendNotification);
    }
    m_degradeKnob->onValueChanged = [this](float val) {
        if (auto* param = m_processor.getApvts().getParameter("degrade")) {
            param->setValueNotifyingHost(val);
        }
    };
    m_macroPanel.addAndMakeVisible(*m_degradeKnob);

    // 12 Module Knobs
    createKnob("sat", "Saturation");
    createKnob("wow", "Wow");
    createKnob("flutter", "Flutter");
    createKnob("drift", "Pitch Drift");
    createKnob("noise", "Noise Floor");
    createKnob("hiss", "Tape Hiss");
    createKnob("dropouts", "Dropouts");
    createKnob("lofi", "Lo-Fi");
    createKnob("bitcrush", "Bitcrush");
    createKnob("hfloss", "HF Loss");
    createKnob("stereodrift", "Stereo Drift");
    createKnob("warble", "Warble");

    // Master Knobs
    auto setupMasterKnob = [this](KnobControl& kc, const std::string& id, const juce::String& name) {
        kc.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        kc.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        kc.label.setText(name, juce::dontSendNotification);
        kc.label.setJustificationType(juce::Justification::centred);
        kc.label.setFont(juce::Font(10.0f, juce::Font::plain));
        kc.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
        kc.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(m_processor.getApvts(), id, kc.slider);
        m_masterPanel.addAndMakeVisible(kc.slider);
        m_masterPanel.addAndMakeVisible(kc.label);
    };

    setupMasterKnob(m_inGainKnob, "inGain", "IN GAIN");
    setupMasterKnob(m_outGainKnob, "outGain", "OUT GAIN");
    setupMasterKnob(m_mixKnob, "mix", "MIX");

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(880, 520);
    startTimerHz(30);
}

VHSPluginEditor::~VHSPluginEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void VHSPluginEditor::createKnob(const std::string& id, const juce::String& name, const juce::String&) {
    auto& kc = m_knobs[id];
    kc.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    kc.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    kc.label.setText(name, juce::dontSendNotification);
    kc.label.setJustificationType(juce::Justification::centred);
    kc.label.setFont(juce::Font(10.0f, juce::Font::plain));
    kc.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    kc.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(m_processor.getApvts(), id, kc.slider);
    m_modulesPanel.addAndMakeVisible(kc.slider);
    m_modulesPanel.addAndMakeVisible(kc.label);
}

void VHSPluginEditor::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    // Background gradient: Deep black with subtle golden ambient gradient
    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::DeepBlack), 0, 0,
                                juce::Colour(0xFF070708), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // Header brand logo & titles
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("ff360_labs", 24, 16, 120, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextOffWhite));
    g.setFont(juce::Font(18.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("VHS", 140, 17, 60, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("TAPE / CASSETTE DEGRADATION SUITE", 200, 21, 260, 18, juce::Justification::left);

    // Top gold divider hairline
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void VHSPluginEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    // Layout: 3 Columns (Macro: 200px, Modules: 480px, Master/Meter: 140px)
    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int macroW = 190;
    const int masterW = 160;
    const int modulesW = getWidth() - macroW - masterW - (pad * 4);

    m_macroPanel.setBounds(pad, top, macroW, contentH);
    m_modulesPanel.setBounds(pad * 2 + macroW, top, modulesW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Hero Knob in Macro Panel
    m_degradeKnob->setBounds(15, 60, macroW - 30, macroW - 10);

    // Modules Panel: 4 Columns x 3 Rows
    static const std::vector<std::string> knobIds = {
        "sat", "wow", "flutter", "drift",
        "noise", "hiss", "dropouts", "lofi",
        "bitcrush", "hfloss", "stereodrift", "warble"
    };

    const int gridCols = 4;
    const int gridRows = 3;
    const int cellW = (modulesW - 20) / gridCols;
    const int cellH = (contentH - 45) / gridRows;

    for (size_t i = 0; i < knobIds.size(); ++i) {
        const int col = static_cast<int>(i % gridCols);
        const int row = static_cast<int>(i / gridCols);
        const int x = 10 + col * cellW;
        const int y = 35 + row * cellH;

        auto& kc = m_knobs[knobIds[i]];
        const int knobSize = std::min(cellW - 10, cellH - 24);
        kc.slider.setBounds(x + (cellW - knobSize) / 2, y, knobSize, knobSize);
        kc.label.setBounds(x, y + knobSize + 2, cellW, 14);
    }

    // Master Panel layout
    m_inGainKnob.slider.setBounds(15, 35, 60, 60);
    m_inGainKnob.label.setBounds(10, 95, 70, 14);

    m_outGainKnob.slider.setBounds(85, 35, 60, 60);
    m_outGainKnob.label.setBounds(80, 95, 70, 14);

    m_mixKnob.slider.setBounds(50, 115, 60, 60);
    m_mixKnob.label.setBounds(45, 175, 70, 14);

    m_meterView.setBounds(30, 205, masterW - 60, contentH - 225);
}

void VHSPluginEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
