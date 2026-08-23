#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

RetroFXEditor::RetroFXEditor(RetroFXProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_heroPanel("GENERATE TRANSITION"),
      m_controlsPanel("GENERATOR SETTINGS"),
      m_masterPanel("OUTPUT / METERING"),
      m_generateHeroKnob("INTENSITY", "%") {
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
    addAndMakeVisible(m_controlsPanel);
    addAndMakeVisible(m_masterPanel);

    // Generator Box
    static const juce::StringArray genNames = {
        "Noise Sweep", "Pitch Sweep", "Laser", "Reverse",
        "Impact", "Riser", "Downlifter", "Digital Sweep", "Tape Sweep"
    };
    for (int i = 0; i < genNames.size(); ++i) {
        m_generatorBox.addItem(genNames[i], i + 1);
    }
    m_genAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "generator", m_generatorBox);
    m_heroPanel.addAndMakeVisible(m_generatorBox);

    // Hero Knob
    if (auto* param = m_processor.getApvts().getRawParameterValue("intensity")) {
        m_generateHeroKnob.setValue(param->load() * 0.01f, juce::dontSendNotification);
    }
    m_generateHeroKnob.onValueChanged = [this](float val) {
        if (auto* param = m_processor.getApvts().getParameter("intensity")) {
            param->setValueNotifyingHost(val);
        }
    };
    m_heroPanel.addAndMakeVisible(m_generateHeroKnob);

    // Generate Button
    m_generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_generateButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF000000));
    m_generateButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF000000));
    m_generateButton.onClick = [this]() {
        m_processor.triggerGenerate();
    };
    m_heroPanel.addAndMakeVisible(m_generateButton);

    // Sync Box
    static const juce::StringArray syncNames = { "Free (2s)", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars", "8 Bars" };
    for (int i = 0; i < syncNames.size(); ++i) {
        m_syncBox.addItem(syncNames[i], i + 1);
    }
    m_syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "sync", m_syncBox);
    m_controlsPanel.addAndMakeVisible(m_syncBox);

    createKnob("seed", "SEED");
    createKnob("mix", "MIX");

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(880, 500);
    startTimerHz(30);
}

RetroFXEditor::~RetroFXEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void RetroFXEditor::createKnob(const std::string& id, const juce::String& name) {
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

void RetroFXEditor::paint(juce::Graphics& g) {
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
    g.drawText("RetroFX", 140, 17, 120, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("GENERATIVE SYNTHWAVE RISERS & TRANSITIONS", 230, 21, 300, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void RetroFXEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int heroW = 280;
    const int masterW = 150;
    const int controlsW = getWidth() - heroW - masterW - (pad * 4);

    m_heroPanel.setBounds(pad, top, heroW, contentH);
    m_controlsPanel.setBounds(pad * 2 + heroW, top, controlsW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Hero Panel
    m_generatorBox.setBounds(20, 35, heroW - 40, 26);
    m_generateHeroKnob.setBounds((heroW - 140) / 2, 75, 140, 150);
    m_generateButton.setBounds(30, contentH - 55, heroW - 60, 36);

    // Controls Panel
    m_syncBox.setBounds(20, 45, controlsW - 40, 26);

    auto& seedKnob = m_knobs["seed"];
    seedKnob.slider.setBounds((controlsW - 70) / 2, 110, 70, 70);
    seedKnob.label.setBounds(10, 185, controlsW - 20, 14);

    // Master Panel
    auto& mixKnob = m_knobs["mix"];
    mixKnob.slider.setBounds((masterW - 64) / 2, 40, 64, 64);
    mixKnob.label.setBounds(10, 108, masterW - 20, 14);

    m_meterView.setBounds(25, 145, masterW - 50, contentH - 165);
}

void RetroFXEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);
}

#endif
