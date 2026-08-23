#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginEditor.h"

NeonWidthEditor::NeonWidthEditor(NeonWidthProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_scopePanel("STEREO FIELD / GONIOMETER"),
      m_imagingPanel("STEREO IMAGING & MOVEMENT"),
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

    addAndMakeVisible(m_scopePanel);
    addAndMakeVisible(m_imagingPanel);
    addAndMakeVisible(m_masterPanel);

    m_scopePanel.addAndMakeVisible(m_stereoFieldView);

    createKnob("microdelay", "MICRO-DELAY");
    createKnob("haas", "HAAS");
    createKnob("detune", "DETUNE");
    createKnob("mswidth", "M/S WIDTH");
    createKnob("freqwidth", "FREQ WIDTH");
    createKnob("freqcrossover", "CROSSOVER");
    createKnob("rotation", "ROTATION");
    createKnob("bassmono", "BASS MONO");
    createKnob("mix", "MIX");

    m_masterPanel.addAndMakeVisible(m_meterView);

    setSize(920, 520);
    startTimerHz(30);
}

NeonWidthEditor::~NeonWidthEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NeonWidthEditor::createKnob(const std::string& id, const juce::String& name) {
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
        m_imagingPanel.addAndMakeVisible(kc.slider);
        m_imagingPanel.addAndMakeVisible(kc.label);
    }
}

void NeonWidthEditor::paint(juce::Graphics& g) {
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
    g.drawText("Neon Width", 140, 17, 120, 24, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("STEREO MANIPULATION & MOVEMENT", 265, 21, 260, 18, juce::Justification::left);

    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.2f));
    g.drawLine(24.0f, 52.0f, bounds.getWidth() - 24.0f, 52.0f, 1.0f);
}

void NeonWidthEditor::resized() {
    const int pad = 20;
    const int top = 64;
    const int contentH = getHeight() - top - pad;

    m_presetBox.setBounds(getWidth() - 220, 16, 196, 26);

    const int scopeW = 280;
    const int masterW = 150;
    const int imagingW = getWidth() - scopeW - masterW - (pad * 4);

    m_scopePanel.setBounds(pad, top, scopeW, contentH);
    m_imagingPanel.setBounds(pad * 2 + scopeW, top, imagingW, contentH);
    m_masterPanel.setBounds(getWidth() - masterW - pad, top, masterW, contentH);

    // Goniometer in Scope Panel
    m_stereoFieldView.setBounds(15, 35, scopeW - 30, contentH - 50);

    // Imaging Panel: 4 cols x 2 rows
    static const std::vector<std::string> knobList = {
        "microdelay", "haas", "detune", "mswidth",
        "freqwidth", "freqcrossover", "rotation", "bassmono"
    };

    const int cols = 4;
    const int rows = 2;
    const int cellW = (imagingW - 20) / cols;
    const int cellH = (contentH - 45) / rows;

    for (size_t i = 0; i < knobList.size(); ++i) {
        const int col = static_cast<int>(i % cols);
        const int row = static_cast<int>(i / cols);
        const int x = 10 + col * cellW;
        const int y = 35 + row * cellH;

        auto& kc = m_knobs[knobList[i]];
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

void NeonWidthEditor::timerCallback() {
    const auto levels = m_processor.getMeteringBridge().getLevels();
    m_meterView.setLevels(levels.peakL, levels.peakR, levels.rmsL, levels.rmsR);

    const auto metrics = m_processor.getStereoEngine().getMetrics();
    const auto& lBuf = m_processor.getLatestLeftBuffer();
    const auto& rBuf = m_processor.getLatestRightBuffer();
    m_stereoFieldView.updateData(metrics.phaseCorrelation, metrics.balance, lBuf.data(), rBuf.data(), lBuf.size());
}

#endif
