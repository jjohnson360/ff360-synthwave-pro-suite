#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

NeonWidthEditor::NeonWidthEditor(NeonWidthProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p)
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    createKnob("haas", "HAAS");
    createKnob("microdelay", "MICRO DELAY");
    createKnob("detune", "DETUNE");
    createKnob("rotation", "ROTATION");
    createKnob("freqcrossover", "CROSSOVER");
    createKnob("mix", "MIX");
    createKnob("freqwidth", "FREQ WIDTH");

    createFader("mswidth", "WIDTH");

    m_bassMonoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_bassMonoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_bassMonoSlider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    m_bassMonoAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "bassmono", m_bassMonoSlider);
    
    m_bassMonoLabel.setText("BASS MONO", juce::dontSendNotification);
    m_bassMonoLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    m_bassMonoLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    m_bassMonoValueLabel.setText("120 Hz", juce::dontSendNotification); // would be updated via listener normally
    m_bassMonoValueLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    m_bassMonoValueLabel.setJustificationType(juce::Justification::centredRight);
    
    addAndMakeVisible(m_bassMonoSlider);
    addAndMakeVisible(m_bassMonoLabel);
    addAndMakeVisible(m_bassMonoValueLabel);

    setSize(350, 640);
}

NeonWidthEditor::~NeonWidthEditor() {
    setLookAndFeel(nullptr);
}

void NeonWidthEditor::createKnob(const std::string& id, const juce::String& name) {
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

void NeonWidthEditor::createFader(const std::string& id, const juce::String& name) {
    auto& f = m_faders[id];
    f.slider.setSliderStyle(juce::Slider::LinearVertical);
    f.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    f.slider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::AccessibleSky));
    f.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, f.slider);
    
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::centred);
    f.label.setFont(juce::Font(8.5f, juce::Font::plain));
    f.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
}

void NeonWidthEditor::paint(juce::Graphics& g) {
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
    g.drawText("4 * STEREO IMAGING & MOVEMENT", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("NEON WIDTH", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void NeonWidthEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    auto mainControls = inner.removeFromTop(250);
    auto rightFader = mainControls.removeFromRight(40);
    
    // Fader
    auto& f = m_faders["mswidth"];
    f.slider.setBounds(rightFader.withTrimmedTop(16).withTrimmedBottom(20).withWidth(20).withX(rightFader.getX() + 10));
    f.label.setBounds(rightFader.getX(), f.slider.getBottom() + 4, rightFader.getWidth(), 14);
    
    // Knobs
    int knobW = 42;
    int knobH = 42;
    int spacingX = (mainControls.getWidth() - (3 * knobW)) / 3;
    int spacingY = 16;
    
    const char* r1[] = {"haas", "microdelay", "freqwidth"};
    for (int i = 0; i < 3; ++i) {
        auto& k = m_knobs[r1[i]];
        int x = mainControls.getX() + spacingX/2 + i * (knobW + spacingX);
        int y = mainControls.getY();
        k.slider.setBounds(x, y, knobW, knobH);
        k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    const char* r2[] = {"detune", "rotation", "mix"};
    for (int i = 0; i < 3; ++i) {
        auto& k = m_knobs[r2[i]];
        int x = mainControls.getX() + spacingX/2 + i * (knobW + spacingX);
        int y = mainControls.getY() + knobH + spacingY + 14;
        k.slider.setBounds(x, y, knobW, knobH);
        k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
    }

    const char* r3[] = {"freqcrossover"};
    for (int i = 0; i < 1; ++i) {
        auto& k = m_knobs[r3[i]];
        int x = mainControls.getX() + spacingX/2 + i * (knobW + spacingX);
        int y = mainControls.getY() + 2*(knobH + spacingY + 14);
        k.slider.setBounds(x, y, knobW, knobH);
        k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(10);
    
    // Bass Mono slider
    auto bassRow = inner.removeFromTop(20);
    m_bassMonoLabel.setBounds(bassRow.getX(), bassRow.getY(), 60, 12);
    m_bassMonoValueLabel.setBounds(bassRow.getRight() - 50, bassRow.getY(), 50, 12);
    m_bassMonoSlider.setBounds(bassRow.getX(), bassRow.getY() + 14, bassRow.getWidth(), 6);
}

#endif
