#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

NightDriveEditor::NightDriveEditor(NightDriveProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_heroPanel("EVOLUTION MACRO"),
      m_layersPanel("AMBIENT LAYERS & SCALE"),
      m_masterPanel("OUTPUT / METERING"),
      m_evolveHeroKnob("EVOLVE", "%") {
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

    addAndMakeVisible(m_heroPanel);
    addAndMakeVisible(m_layersPanel);
    addAndMakeVisible(m_masterPanel);

    // Hero Knob
    m_evolveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "evolve", m_evolveHeroKnob.getSlider());
    m_heroPanel.addAndMakeVisible(m_evolveHeroKnob);

    // Scale Box
    static const juce::StringArray scaleNames = {
        "Major", "Natural Minor", "Dorian", "Phrygian",
        "Lydian", "Mixolydian", "Synthwave Pentatonic", "ChordFlow (Auto)"
    };
    for (int i = 0; i < scaleNames.size(); ++i) {
        m_scaleBox.addItem(scaleNames[i], i + 1);
    }
    m_scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "scalelock", m_scaleBox);
    m_layersPanel.addAndMakeVisible(m_scaleBox);

    // ChordFlow status readout
    m_chordFlowStatus.setText("ChordFlow: Standalone (Fallback)", juce::dontSendNotification);
    m_chordFlowStatus.setFont(juce::Font(juce::FontOptions().withHeight(9.5f)));
    m_chordFlowStatus.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    m_layersPanel.addAndMakeVisible(m_chordFlowStatus);

    // Knobs
    createKnob("dronelevel", "DRONE");
    createKnob("granularlevel", "GRANULAR");
    createKnob("arplevel", "ARP MOTION");
    createKnob("density", "DENSITY");
    createKnob("filtermove", "FILTER LFO");
    createKnob("reverbwash", "REVERB WASH");
    createKnob("mix", "MIX");

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(920, 520);
    startTimerHz(30);
}

NightDriveEditor::~NightDriveEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NightDriveEditor::createKnob(const std::string& id, const juce::String& name) {
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
        m_layersPanel.addAndMakeVisible(kc.slider);
        m_layersPanel.addAndMakeVisible(kc.label);
    }
}

void NightDriveEditor::paint(juce::Graphics& g) {
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
    g.drawText("NightDrive", 140, 17, 140, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("GENERATIVE SYNTHWAVE AMBIENT BED & TEXTURES", 270, 21, 320, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void NightDriveEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int heroW = 260;
    const int masterW = 150;
    const int layersW = getWidth() - heroW - masterW - (pad * 4);

    m_heroPanel.setBounds(pad, top, heroW, contentH);
    m_layersPanel.setBounds(pad * 2 + heroW, top, layersW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Hero Panel
    m_evolveHeroKnob.setBounds((heroW - 160) / 2, (contentH - 170) / 2, 160, 170);

    // Layers Panel
    m_scaleBox.setBounds(15, 35, 180, 26);
    m_chordFlowStatus.setBounds(205, 35, 200, 26);

    // Knobs: 3 cols x 2 rows
    static const std::vector<std::string> layerList = {
        "dronelevel", "granularlevel", "arplevel",
        "density", "filtermove", "reverbwash"
    };

    const int cols = 3;
    const int rows = 2;
    const int cellW = (layersW - 20) / cols;
    const int cellH = (contentH - 80) / rows;

    for (size_t i = 0; i < layerList.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 75 + row * cellH;

        auto& kc = m_knobs[layerList[i]];
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

void NightDriveEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);

    if (m_processor.isChordFlowActive()) {
        m_chordFlowStatus.setText("ChordFlow: Connected (Live)", juce::dontSendNotification);
        m_chordFlowStatus.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::AccessibleSkyBlue));
    } else {
        m_chordFlowStatus.setText("ChordFlow: Standalone (Fallback)", juce::dontSendNotification);
        m_chordFlowStatus.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    }
}

#endif
