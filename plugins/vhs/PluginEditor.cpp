#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

VHSPluginEditor::VHSPluginEditor(VHSPluginProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p)
{
    setLookAndFeel(&m_lookAndFeel);

    // Main Panel
    addAndMakeVisible(m_mainPanel);
    
    // Scene
    addAndMakeVisible(m_scene);

    // Hero Knob
    m_degradeKnob = std::make_unique<ff360_ui::FF360_HeroKnob>("DEGRADE");
    if (auto* param = m_processor.getApvts().getRawParameterValue("degrade")) {
        m_degradeKnob->setValue(param->load() * 0.01f, juce::dontSendNotification);
    }
    m_degradeKnob->onValueChanged = [this](float val) {
        if (auto* param = m_processor.getApvts().getParameter("degrade")) {
            param->setValueNotifyingHost(val);
        }
    };
    addAndMakeVisible(m_degradeKnob.get());

    m_ff360Label.setText("ff360_labs", juce::dontSendNotification);
    m_ff360Label.setJustificationType(juce::Justification::centredRight);
    m_ff360Label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    m_ff360Label.setFont(juce::Font(9.0f, juce::Font::plain));
    addAndMakeVisible(m_ff360Label);

    // 10 parameter knobs (2 rows of 5)
    createKnob("wow", "Wow");
    createKnob("flutter", "Flutter");
    createKnob("noise", "Noise");
    createKnob("hiss", "Hiss");
    createKnob("dropouts", "Dropouts");
    
    createKnob("saturation", "Saturate");
    createKnob("bitcrush", "Bit Red.");
    createKnob("hpf", "HF Rolloff");
    createKnob("stereo_drift", "St. Drift");
    createKnob("pitch_drift", "Pitch Drift");

    // Mode Selector (dummy buttons for now)
    m_modePrevButton.setButtonText("<");
    m_modeNextButton.setButtonText(">");
    m_modeLabel.setText("MODE - VHS", juce::dontSendNotification);
    m_modeLabel.setJustificationType(juce::Justification::centred);
    m_modeLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    m_modeLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    
    addAndMakeVisible(m_modePrevButton);
    addAndMakeVisible(m_modeNextButton);
    addAndMakeVisible(m_modeLabel);

    // Mix Slider
    m_mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "mix", m_mixSlider);
    
    m_mixLabel.setText("MIX", juce::dontSendNotification);
    m_mixLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    m_mixLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    m_mixValueLabel.setText("100%", juce::dontSendNotification);
    m_mixValueLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    m_mixValueLabel.setJustificationType(juce::Justification::centredRight);
    
    addAndMakeVisible(m_mixSlider);
    addAndMakeVisible(m_mixLabel);
    addAndMakeVisible(m_mixValueLabel);

    setSize(350, 640);
}

VHSPluginEditor::~VHSPluginEditor() {
    setLookAndFeel(nullptr);
}

void VHSPluginEditor::createKnob(const std::string& id, const juce::String& name) {
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

void VHSPluginEditor::paint(juce::Graphics& g) {
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
    g.drawText("1 * TAPE DEGRADATION", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("VHS", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void VHSPluginEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24); // header space
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    // Hero Knob Row
    auto heroRow = inner.removeFromTop(96);
    m_degradeKnob->setBounds(heroRow.removeFromLeft(96));
    
    m_ff360Label.setBounds(heroRow.removeFromRight(80).withHeight(20).translated(0, 40));
    inner.removeFromTop(16);
    
    // Knob Rows (5 knobs per row)
    int knobW = 42;
    int knobH = 42;
    int spacing = (inner.getWidth() - (5 * knobW)) / 4;
    
    auto row1 = inner.removeFromTop(60);
    const char* r1[] = {"wow", "flutter", "noise", "hiss", "dropouts"};
    for (int i = 0; i < 5; ++i) {
        auto& k = m_knobs[r1[i]];
        k.slider.setBounds(row1.getX() + i * (knobW + spacing), row1.getY(), knobW, knobH);
        k.label.setBounds(k.slider.getX() - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(12);
    auto row2 = inner.removeFromTop(60);
    const char* r2[] = {"saturation", "bitcrush", "hpf", "stereo_drift", "pitch_drift"};
    for (int i = 0; i < 5; ++i) {
        auto& k = m_knobs[r2[i]];
        k.slider.setBounds(row2.getX() + i * (knobW + spacing), row2.getY(), knobW, knobH);
        k.label.setBounds(k.slider.getX() - 10, k.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(20);
    
    // Paginator
    auto pageRow = inner.removeFromTop(20);
    m_modePrevButton.setBounds(pageRow.getCentreX() - 50, pageRow.getY(), 20, 20);
    m_modeLabel.setBounds(pageRow.getCentreX() - 30, pageRow.getY(), 60, 20);
    m_modeNextButton.setBounds(pageRow.getCentreX() + 30, pageRow.getY(), 20, 20);
    
    inner.removeFromTop(12);
    
    // Mix Row
    auto mixRow = inner.removeFromTop(20);
    m_mixLabel.setBounds(mixRow.getX(), mixRow.getY(), 40, 12);
    m_mixValueLabel.setBounds(mixRow.getRight() - 40, mixRow.getY(), 40, 12);
    m_mixSlider.setBounds(mixRow.getX(), mixRow.getY() + 14, mixRow.getWidth(), 6);
}

#endif
