#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

RetroFXEditor::RetroFXEditor(RetroFXProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p)
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    // Generator Type (We use a combobox to represent the list in a compact way)
    m_generatorBox.addItem("Noise Sweep", 1);
    m_generatorBox.addItem("Pitch Sweep", 2);
    m_generatorBox.addItem("Laser", 3);
    m_generatorBox.addItem("Reverse", 4);
    m_generatorBox.addItem("Impact", 5);
    m_generatorBox.addItem("Riser", 6);
    m_generatorBox.addItem("Downlifter", 7);
    m_generatorBox.addItem("Digital Sweep", 8);
    m_generatorBox.addItem("Tape Sweep", 9);
    m_generatorBox.setJustificationType(juce::Justification::centred);
    m_genAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "generator", m_generatorBox);
    addAndMakeVisible(m_generatorBox);

    // Sync
    m_syncBox.addItem("Free (2s)", 1);
    m_syncBox.addItem("1/2 Bar", 2);
    m_syncBox.addItem("1 Bar", 3);
    m_syncBox.addItem("2 Bars", 4);
    m_syncBox.addItem("4 Bars", 5);
    m_syncBox.addItem("8 Bars", 6);
    m_syncBox.setJustificationType(juce::Justification::centred);
    m_syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "sync", m_syncBox);
    addAndMakeVisible(m_syncBox);

    createKnob("intensity", "INTENSITY");
    createKnob("startpitch", "START PITCH");
    createKnob("endpitch", "END PITCH");
    createKnob("length", "LENGTH");
    createKnob("seed", "SEED");

    // Mix fader
    m_mixSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_mixSlider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "mix", m_mixSlider);
    m_mixLabel.setText("MIX", juce::dontSendNotification);
    m_mixLabel.setJustificationType(juce::Justification::centred);
    m_mixLabel.setFont(juce::Font(8.5f, juce::Font::plain));
    m_mixLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    addAndMakeVisible(m_mixSlider);
    addAndMakeVisible(m_mixLabel);

    // Generate Button
    m_generateBtn.setButtonText("GENERATE FX");
    addAndMakeVisible(m_generateBtn);

    setSize(350, 640);
}

RetroFXEditor::~RetroFXEditor() {
    setLookAndFeel(nullptr);
}

void RetroFXEditor::createKnob(const std::string& id, const juce::String& name) {
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

void RetroFXEditor::paint(juce::Graphics& g) {
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
    g.drawText("5 * TRANSITIONS & FX GENERATOR", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("RETROFX", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void RetroFXEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    // Type and Sync
    m_generatorBox.setBounds(inner.removeFromTop(30));
    inner.removeFromTop(12);
    m_syncBox.setBounds(inner.removeFromTop(30));
    inner.removeFromTop(16);
    
    auto controls = inner.removeFromTop(160);
    
    // Mix fader on the right
    auto rightFader = controls.removeFromRight(40);
    m_mixSlider.setBounds(rightFader.withTrimmedTop(16).withTrimmedBottom(20).withWidth(20).withX(rightFader.getX() + 10));
    m_mixLabel.setBounds(rightFader.getX(), m_mixSlider.getBottom() + 4, rightFader.getWidth(), 14);
    
    // Knobs: row 1 = Intensity / Start Pitch / End Pitch, row 2 = Length / Seed
    int knobW = 42;
    int knobH = 42;

    auto layoutRow = [&](const char* const* ids, int count, int y) {
        int dx = (controls.getWidth() - (count * knobW)) / (count + 1);
        for (int i = 0; i < count; ++i) {
            auto& k = m_knobs[ids[i]];
            int x = controls.getX() + dx + i * (knobW + dx);
            k.slider.setBounds(x, y, knobW, knobH);
            k.label.setBounds(x - 10, k.slider.getBottom(), knobW + 20, 14);
        }
    };

    const char* row1[] = { "intensity", "startpitch", "endpitch" };
    const char* row2[] = { "length", "seed" };
    layoutRow(row1, 3, controls.getY() + 6);
    layoutRow(row2, 2, controls.getY() + 6 + knobH + 28);
    
    // Generate Button
    inner.removeFromTop(20);
    m_generateBtn.setBounds(inner.removeFromTop(40).reduced(20, 0));
}

#endif
