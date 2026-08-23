#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

MidnightReverbEditor::MidnightReverbEditor(MidnightReverbProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_decayPanel("ALGORITHM & DECAY"),
      m_controlsPanel("REVERB SHAPING & DAMPING"),
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

    addAndMakeVisible(m_decayPanel);
    addAndMakeVisible(m_controlsPanel);
    addAndMakeVisible(m_masterPanel);

    // Algorithm combo box
    static const juce::StringArray algNames = { "Digital Hall", "Dark Plate", "Gated Room", "Synth Room", "Endless", "Dream" };
    for (int i = 0; i < algNames.size(); ++i) {
        m_algBox.addItem(algNames[i], i + 1);
    }
    m_algAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "alg", m_algBox);
    m_decayPanel.addAndMakeVisible(m_algBox);

    // Hero Decay Knob
    m_decayHeroKnob = std::make_unique<ff360_ui::FF360_HeroKnob>("DECAY", "%");
    if (auto* param = m_processor.getApvts().getRawParameterValue("decay")) {
        m_decayHeroKnob->setValue(param->load() * 0.01f, juce::dontSendNotification);
    }
    m_decayHeroKnob->onValueChanged = [this](float val) {
        if (auto* param = m_processor.getApvts().getParameter("decay")) {
            param->setValueNotifyingHost(val);
        }
    };
    m_decayPanel.addAndMakeVisible(*m_decayHeroKnob);

    // Reverb Controls
    createKnob("predelay", "PRE-DELAY");
    createKnob("ducking", "DUCKING");
    createKnob("width", "WIDTH");
    createKnob("modulation", "MODULATION");
    createKnob("lowdamp", "LOW CUT");
    createKnob("highdamp", "HI DAMP");
    createKnob("mix", "MIX");

    // Toggles
    m_syncToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_syncToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "synctoggle", m_syncToggle);
    m_controlsPanel.addAndMakeVisible(m_syncToggle);

    m_freezeToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(ff360_ui::Colors::TextOffWhite));
    m_freezeToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "freeze", m_freezeToggle);
    m_decayPanel.addAndMakeVisible(m_freezeToggle);

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(880, 520);
    startTimerHz(30);
}

MidnightReverbEditor::~MidnightReverbEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void MidnightReverbEditor::createKnob(const std::string& id, const juce::String& name) {
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

void MidnightReverbEditor::paint(juce::Graphics& g) {
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
    g.drawText("Midnight Reverb", 140, 17, 160, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("CINEMATIC RETRO DIGITAL SPACES", 305, 21, 250, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void MidnightReverbEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int decayW = 210;
    const int masterW = 150;
    const int controlsW = getWidth() - decayW - masterW - (pad * 4);

    m_decayPanel.setBounds(pad, top, decayW, contentH);
    m_controlsPanel.setBounds(pad * 2 + decayW, top, controlsW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Decay Panel
    m_algBox.setBounds(15, 35, decayW - 30, 26);
    m_decayHeroKnob->setBounds(15, 75, decayW - 30, decayW - 15);
    m_freezeToggle.setBounds(20, contentH - 45, decayW - 40, 26);

    // Controls Panel: 3 cols x 2 rows + Sync Toggle
    static const std::vector<std::string> controlKnobs = {
        "predelay", "ducking", "width",
        "modulation", "lowdamp", "highdamp"
    };

    const int cols = 3;
    const int rows = 2;
    const int cellW = (controlsW - 20) / cols;
    const int cellH = (contentH - 70) / rows;

    for (size_t i = 0; i < controlKnobs.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 35 + row * cellH;

        auto& kc = m_knobs[controlKnobs[i]];
        const int knobSize = std::min(cellW - 10, cellH - 24);
        kc.slider.setBounds(x + (cellW - knobSize) / 2, y, knobSize, knobSize);
        kc.label.setBounds(x, y + knobSize + 2, cellW, 14);
    }

    m_syncToggle.setBounds(15, contentH - 38, 140, 26);

    // Master Panel
    auto& mixKnob = m_knobs["mix"];
    mixKnob.slider.setBounds((masterW - 64) / 2, 40, 64, 64);
    mixKnob.label.setBounds(10, 108, masterW - 20, 14);

    m_meterView.setBounds(25, 145, masterW - 50, contentH - 165);
}

void MidnightReverbEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
