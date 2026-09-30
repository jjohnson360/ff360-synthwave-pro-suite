#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

CyberpunkGlitchEditor::CyberpunkGlitchEditor(CyberpunkGlitchProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_workflowBar(p.getHistory(), p.getPresetManager())
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_workflowBar);
    m_workflowBar.attachKeyboardShortcuts(*this);

    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    m_divisionBox.addItem("1/4", 1);
    m_divisionBox.addItem("1/8", 2);
    m_divisionBox.addItem("1/16", 3);
    m_divisionBox.addItem("1/32", 4);
    m_divisionBox.setJustificationType(juce::Justification::centred);
    m_divisionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "division", m_divisionBox);
    addAndMakeVisible(m_divisionBox);

    m_freezeToggle.setButtonText("Freeze");
    m_freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "freeze", m_freezeToggle);
    addAndMakeVisible(m_freezeToggle);

    m_reverseToggle.setButtonText("Reverse");
    m_reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "reverse", m_reverseToggle);
    addAndMakeVisible(m_reverseToggle);

    createHFader("probability", "PROBABILITY", true);
    createHFader("filter", "FILTER");

    createKnob("bitcrush", "BITCRUSH");
    createKnob("pitch", "PITCH", true);
    createKnob("gate", "GATE");
    createKnob("resonance", "RESONANCE");
    createKnob("mix", "MIX");
    createKnob("scsensitivity", "SC TRIG");

    setSize(350, 676);
    startTimerHz(30);
}

CyberpunkGlitchEditor::~CyberpunkGlitchEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void CyberpunkGlitchEditor::timerCallback() {
    auto& engine = m_processor.getGlitchEngine();
    m_scene.stepHistory = engine.getStepHistoryOldestFirst();
    m_scene.isLive = engine.isGlitching();
    m_scene.gridProgress = engine.getGridProgress();
    m_scene.repaint();
}

void CyberpunkGlitchEditor::createKnob(const std::string& id, const juce::String& name, bool amber) {
    auto& k = m_knobs[id];
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    // Quick and dirty "amber" flag using LookAndFeel thumb colour override logic
    if (amber) {
        k.slider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    }
    
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, k.slider);
    
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(8.5f, juce::Font::plain));
    k.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
}

void CyberpunkGlitchEditor::createHFader(const std::string& id, const juce::String& name, bool amber) {
    auto& f = m_faders[id];
    f.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    f.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    f.slider.setColour(juce::Slider::thumbColourId, juce::Colour(amber ? ff360_ui::Colors::WarmAmberRed : ff360_ui::Colors::AccessibleSky));
    
    f.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, f.slider);
    
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::left);
    f.label.setFont(juce::Font(9.0f, juce::Font::bold));
    f.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    f.valueLabel.setText("-", juce::dontSendNotification);
    f.valueLabel.setJustificationType(juce::Justification::centredRight);
    f.valueLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    f.valueLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
    addAndMakeVisible(f.valueLabel);
}

void CyberpunkGlitchEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::MatteCharcoal), 0, 0,
                                juce::Colour(0xFF131316), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();
    
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.16f));
    g.drawRect(bounds, 1.0f);
    
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.drawText("8 * BEAT-SYNCED GLITCH EFFECTS", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("CYBERPUNK GLITCH", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void CyberpunkGlitchEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    bounds.removeFromTop(4);
    m_workflowBar.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(6);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Top row controls
    auto paginator = inner.removeFromTop(24);
    m_divisionBox.setBounds(paginator.removeFromRight(100));
    m_freezeToggle.setBounds(paginator.removeFromLeft(60));
    m_reverseToggle.setBounds(paginator.removeFromLeft(80).withTrimmedLeft(10));
    
    inner.removeFromTop(10);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(100));
    inner.removeFromTop(16);
    
    // Sliders
    auto probRow = inner.removeFromTop(36);
    auto& fp = m_faders["probability"];
    fp.label.setBounds(probRow.getX(), probRow.getY(), 80, 12);
    fp.valueLabel.setBounds(probRow.getRight() - 60, probRow.getY(), 60, 12);
    fp.slider.setBounds(probRow.getX(), probRow.getY() + 16, probRow.getWidth(), 8);
    
    auto filtRow = inner.removeFromTop(36);
    auto& ff = m_faders["filter"];
    ff.label.setBounds(filtRow.getX(), filtRow.getY(), 80, 12);
    ff.valueLabel.setBounds(filtRow.getRight() - 60, filtRow.getY(), 60, 12);
    ff.slider.setBounds(filtRow.getX(), filtRow.getY() + 16, filtRow.getWidth(), 8);
    
    inner.removeFromTop(16);
    
    // Knobs (6 knobs)
    int knobW = 42;
    int knobH = 42;
    const int numKnobs = 6;
    int dx = (inner.getWidth() - (numKnobs * knobW)) / (numKnobs - 1);

    const char* rowK[] = {"bitcrush", "pitch", "gate", "resonance", "mix", "scsensitivity"};
    for(int i = 0; i < numKnobs; ++i) {
        auto& k = m_knobs[rowK[i]];
        int x = inner.getX() + i * (knobW + dx);
        k.slider.setBounds(x, inner.getY(), knobW, knobH);
        k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
    }
}

#endif
