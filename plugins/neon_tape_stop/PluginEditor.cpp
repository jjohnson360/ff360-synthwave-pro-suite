#include "PluginEditor.h"
#include <cmath>

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

NeonTapeStopEditor::NeonTapeStopEditor(NeonTapeStopProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_workflowBar(p.getHistory(), p.getPresetManager()),
      m_outputStrip(p.getApvts(), p.getOutputStage())
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_workflowBar);
    m_workflowBar.attachKeyboardShortcuts(*this);
    addAndMakeVisible(m_outputStrip);

    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    createKnob("slowdown", "STOP TIME");
    createKnob("pitchcurve", "PITCH CRV");
    createKnob("filtermove", "FILTER");
    createKnob("recovery", "RECOVERY");

    m_triggerBtn.setButtonText("TRIGGER STOP");
    m_triggerBtn.setClickingTogglesState(true);
    m_triggerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "trigger", m_triggerBtn);
    addAndMakeVisible(m_triggerBtn);

    m_reverseToggle.setButtonText("Reverse Rec");
    m_reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "reverse", m_reverseToggle);
    addAndMakeVisible(m_reverseToggle);

    m_profileBox.addItem("Vinyl Stop", 1);
    m_profileBox.addItem("Tape Stop", 2);
    m_profileBox.addItem("Digital Stop", 3);
    m_profileBox.setJustificationType(juce::Justification::centred);
    m_profileAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "profile", m_profileBox);
    addAndMakeVisible(m_profileBox);

    m_mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "mix", m_mixSlider);
    m_mixLabel.setText("MIX", juce::dontSendNotification);
    m_mixLabel.setJustificationType(juce::Justification::left);
    m_mixLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    m_mixLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    addAndMakeVisible(m_mixSlider);
    addAndMakeVisible(m_mixLabel);

    setSize(350, 712);
    startTimerHz(30);
}

NeonTapeStopEditor::~NeonTapeStopEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NeonTapeStopEditor::timerCallback() {
    // Drive the reel animation from the real tape-stop state machine rather than
    // a fixed spin rate, so the reels visually slow down / brake / spin back up
    // in sync with what the DSP is actually doing to the audio.
    using ff360::TapeStopState;
    auto& controller = m_processor.getController();
    const float progress = controller.getProgress(); // 0 = running normally, 1 = fully stopped
    const float speedFactor = 1.0f - progress;

    m_scene.speedFactor = speedFactor;
    m_scene.isStopped = (controller.getState() == TapeStopState::Stopped);
    m_scene.angle = std::fmod(m_scene.angle + 0.05f * speedFactor, juce::MathConstants<float>::twoPi);
    m_scene.repaint();
}

void NeonTapeStopEditor::createKnob(const std::string& id, const juce::String& name) {
    auto& k = m_knobs[id];
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, k.slider);
    
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(8.5f, juce::Font::plain));
    k.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
}

void NeonTapeStopEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::MatteCharcoal), 0, 0,
                                juce::Colour(0xFF131316), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();
    
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.16f));
    g.drawRect(bounds, 1.0f);
    
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.drawText("7 * MUSICAL TAPE STOP EFFECT", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("NEON TAPE STOP", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void NeonTapeStopEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    bounds.removeFromTop(4);
    m_workflowBar.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(6);
    m_outputStrip.setBounds(bounds.removeFromBottom(28));
    bounds.removeFromBottom(8);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    // Trigger and profile
    auto topRow = inner.removeFromTop(36);
    m_triggerBtn.setBounds(topRow.removeFromLeft(140));
    m_profileBox.setBounds(topRow.removeFromRight(120).withSizeKeepingCentre(120, 24));
    inner.removeFromTop(24);
    
    // Knobs 2x2
    int knobW = 42;
    int knobH = 42;
    int dx = (inner.getWidth() - (2 * knobW)) / 3;
    int dy = 24;
    
    const char* row1[] = {"slowdown", "pitchcurve"};
    for(int i = 0; i < 2; ++i) {
        auto& k = m_knobs[row1[i]];
        k.slider.setBounds(inner.getX() + dx + i*(knobW + dx), inner.getY(), knobW, knobH);
        k.label.setBounds(k.slider.getX() - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(knobH + dy + 14);
    
    const char* row2[] = {"recovery", "filtermove"};
    for(int i = 0; i < 2; ++i) {
        auto& k = m_knobs[row2[i]];
        k.slider.setBounds(inner.getX() + dx + i*(knobW + dx), inner.getY(), knobW, knobH);
        k.label.setBounds(k.slider.getX() - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(knobH + dy + 14);
    
    // Reverse
    m_reverseToggle.setBounds(inner.removeFromTop(24).withWidth(100).withX(inner.getX()));
    
    inner.removeFromTop(16);
    
    // Mix
    auto mixRow = inner.removeFromTop(20);
    m_mixLabel.setBounds(mixRow.removeFromLeft(40));
    m_mixSlider.setBounds(mixRow.withTrimmedTop(6).withTrimmedBottom(6));
}

#endif
