#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

MidnightReverbEditor::MidnightReverbEditor(MidnightReverbProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p)
{
    setLookAndFeel(&m_lookAndFeel);

    // Main Panel
    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    // Algorithm ComboBox
    m_algBox.addItem("Digital Hall", 1);
    m_algBox.addItem("Dark Plate", 2);
    m_algBox.addItem("Gated Room", 3);
    m_algBox.addItem("Synth Room", 4);
    m_algBox.addItem("Endless", 5);
    m_algBox.addItem("Dream", 6);
    m_algBox.setJustificationType(juce::Justification::centred);
    m_algAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "alg", m_algBox);
    addAndMakeVisible(m_algBox);

    // Knobs
    createKnob("decay", "DECAY");
    createKnob("predelay", "PRE-DELAY");
    createKnob("lowdamp", "LOW CUT");
    createKnob("highdamp", "HIGH DAMP");
    createKnob("width", "WIDTH");

    // Vertical Faders (Bar Meters)
    createFader("ducking", "DUCKING", juce::Colour(ff360_ui::Colors::AccessibleSky));
    createFader("modulation", "MODULATION", juce::Colour(ff360_ui::Colors::WarmAmberRed));
    createFader("mix", "MIX", juce::Colour(ff360_ui::Colors::MetallicGold));

    // Freeze Button
    m_freezeButton.setButtonText("FREEZE");
    m_freezeButton.setClickingTogglesState(true);
    m_freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "freeze", m_freezeButton);
    addAndMakeVisible(m_freezeButton);

    // Tempo Sync toggle (pre-delay locks to host tempo)
    m_syncButton.setButtonText("SYNC");
    m_syncButton.setClickingTogglesState(true);
    m_syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "synctoggle", m_syncButton);
    addAndMakeVisible(m_syncButton);

    setSize(350, 640);
}

MidnightReverbEditor::~MidnightReverbEditor() {
    setLookAndFeel(nullptr);
}

void MidnightReverbEditor::createKnob(const std::string& id, const juce::String& name) {
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

void MidnightReverbEditor::createFader(const std::string& id, const juce::String& name, const juce::Colour& accentColour) {
    auto& f = m_faders[id];
    f.slider.setSliderStyle(juce::Slider::LinearVertical);
    f.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    f.slider.setColour(juce::Slider::thumbColourId, accentColour);
    f.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, f.slider);
    
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::centred);
    f.label.setFont(juce::Font(8.5f, juce::Font::plain));
    f.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
}

void MidnightReverbEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background gradient
    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::MatteCharcoal), 0, 0,
                                juce::Colour(0xFF131316), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();
    
    // Outer border
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.16f));
    g.drawRect(bounds, 1.0f);
    
    // Eyebrow and Title
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.drawText("3 * 80S SPATIAL ENGINE", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("MIDNIGHT REVERB", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void MidnightReverbEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    // Algorithm, Sync, and Freeze row
    auto topRow = inner.removeFromTop(24);
    m_algBox.setBounds(topRow.removeFromLeft(120));
    m_freezeButton.setBounds(topRow.removeFromRight(70));
    m_syncButton.setBounds(topRow.removeFromRight(60).withTrimmedRight(6));
    
    inner.removeFromTop(16);
    
    // Faders side by side on the right, Knobs on the left
    auto rightArea = inner.removeFromRight(120);
    int faderW = 20;
    int faderH = 160;
    int faderSpacing = (rightArea.getWidth() - (3 * faderW)) / 2;
    
    const char* faderNames[] = {"ducking", "modulation", "mix"};
    for (int i = 0; i < 3; ++i) {
        auto& f = m_faders[faderNames[i]];
        int x = rightArea.getX() + i * (faderW + faderSpacing);
        f.slider.setBounds(x, rightArea.getY() + 20, faderW, faderH);
        // Vertical text or small label below
        f.label.setBounds(x - 20, f.slider.getBottom() + 4, faderW + 40, 14);
    }
    
    auto leftArea = inner.removeFromLeft(150);
    int knobW = 42;
    int knobH = 42;
    int dx = (leftArea.getWidth() - (2 * knobW)) / 2;
    int dy = 16;
    
    const char* kNames[] = {"decay", "predelay", "lowdamp", "highdamp"};
    for (int i = 0; i < 4; ++i) {
        auto& k = m_knobs[kNames[i]];
        int r = i / 2;
        int c = i % 2;
        int x = leftArea.getX() + c * (knobW + dx);
        int y = leftArea.getY() + 20 + r * (knobH + dy + 14);
        k.slider.setBounds(x, y, knobW, knobH);
        k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    // 5th knob (width) centered below
    auto& wKnob = m_knobs["width"];
    wKnob.slider.setBounds(leftArea.getCentreX() - knobW/2, leftArea.getY() + 20 + 2 * (knobH + dy + 14), knobW, knobH);
    wKnob.label.setBounds(wKnob.slider.getX() - 10, wKnob.slider.getBottom(), knobW + 20, 14);
}

#endif
