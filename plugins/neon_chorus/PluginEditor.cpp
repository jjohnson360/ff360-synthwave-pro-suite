#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

NeonChorusEditor::NeonChorusEditor(NeonChorusProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_workflowBar(p.getHistory(), p.getPresetManager()),
      m_outputStrip(p.getApvts(), p.getOutputStage())
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_workflowBar);
    m_workflowBar.attachKeyboardShortcuts(*this);
    addAndMakeVisible(m_outputStrip);

    // Main Panel
    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scene);

    // Core Knobs
    createKnob("rate", "RATE");
    createKnob("depth", "DEPTH");
    createKnob("detune", "DETUNE");
    createKnob("feedback", "FEEDBACK", true); // amber accent
    
    createKnob("width", "WIDTH");
    createKnob("predelay", "PRE-DELAY");
    createKnob("mix", "MIX");
    createKnob("bassmono", "BASS MONO");

    // Quad Toggle Button
    m_quadButton.setButtonText("Quad Chorus");
    m_quadButton.setClickingTogglesState(true);
    m_quadAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "quad", m_quadButton);
    addAndMakeVisible(m_quadButton);

    // Vintage/Modern character toggle
    m_vintageButton.setButtonText("Vintage");
    m_vintageButton.setClickingTogglesState(true);
    m_vintageAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        m_processor.getApvts(), "vintage", m_vintageButton);
    addAndMakeVisible(m_vintageButton);

    addAndMakeVisible(m_d1);
    addAndMakeVisible(m_d2);
    addAndMakeVisible(m_d3);


    // Tooltips: every control explains itself on hover (the editor test fails on any without one)
    m_knobs["rate"].slider.setTooltip("Rate: speed of the chorus sweep.");
    m_knobs["depth"].slider.setTooltip("Depth: how far the voices sweep.");
    m_knobs["detune"].slider.setTooltip("Detune: pitch spread between the voices.");
    m_knobs["feedback"].slider.setTooltip("Feedback: feeds the chorus back into itself for a more metallic, flanger-like sound.");
    m_knobs["width"].slider.setTooltip("Width: stereo spread of the voices.");
    m_knobs["predelay"].slider.setTooltip("Pre-delay: delay before the chorus voices, in ms.");
    m_knobs["mix"].slider.setTooltip("Mix: balance between the dry signal and the chorus.");
    m_knobs["bassmono"].slider.setTooltip("Bass Mono: keeps everything below this frequency in mono (0 = off).");
    m_quadButton.setTooltip("Quad Chorus: 4 voices instead of 2.");
    m_vintageButton.setTooltip("Vintage: vintage chorus character. Off gives a cleaner, modern chorus.");

    setSize(350, 712);
}

NeonChorusEditor::~NeonChorusEditor() {
    setLookAndFeel(nullptr);
}

void NeonChorusEditor::createKnob(const std::string& id, const juce::String& name, bool useAmberAccent) {
    auto& k = m_knobs[id];
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    if (useAmberAccent) {
        k.slider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    }
    
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, k.slider);
    
    ff360_ui::showValuePopup(k.slider, m_processor.getApvts(), id, this);
    
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(8.5f, juce::Font::plain));
    k.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
}

void NeonChorusEditor::paint(juce::Graphics& g) {
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
    g.drawText("2 * 80S STEREO CHORUS", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("NEON CHORUS", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void NeonChorusEditor::resized() {
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(24);
    bounds.removeFromTop(4);
    m_workflowBar.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(6);
    m_outputStrip.setBounds(bounds.removeFromBottom(28));
    bounds.removeFromBottom(8);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene
    m_scene.setBounds(inner.removeFromTop(120));
    inner.removeFromTop(16);
    
    // Knobs (2 columns of 4)
    auto knobsArea = inner.removeFromTop(300);
    int colW = knobsArea.getWidth() / 2;
    int knobW = 42;
    int knobH = 42;
    int spacingY = 16;
    
    const char* col1[] = {"rate", "depth", "detune", "feedback"};
    const char* col2[] = {"width", "predelay", "mix", "bassmono"};
    
    for (int i = 0; i < 4; ++i) {
        auto& k1 = m_knobs[col1[i]];
        int x1 = knobsArea.getX() + (colW - knobW) / 2;
        int y1 = knobsArea.getY() + i * (knobH + spacingY + 14);
        k1.slider.setBounds(x1, y1, knobW, knobH);
        k1.label.setBounds(x1 - 10, k1.slider.getBottom(), knobW + 20, 14);
        
        auto& k2 = m_knobs[col2[i]];
        int x2 = knobsArea.getX() + colW + (colW - knobW) / 2;
        int y2 = knobsArea.getY() + i * (knobH + spacingY + 14);
        k2.slider.setBounds(x2, y2, knobW, knobH);
        k2.label.setBounds(x2 - 10, k2.slider.getBottom(), knobW + 20, 14);
    }
    
    inner.removeFromTop(16);
    
    // Divider
    // (We could draw a line here in paint, but leaving space is fine)
    
    // Quad Chorus + Vintage/Modern buttons, side by side
    auto btnArea = inner.removeFromTop(36);
    int btnGap = 8;
    int btnW = (btnArea.getWidth() - btnGap) / 2;
    m_quadButton.setBounds(btnArea.removeFromLeft(btnW));
    btnArea.removeFromLeft(btnGap);
    m_vintageButton.setBounds(btnArea);
    
    inner.removeFromTop(16);
    
    // Dots
    auto dotsArea = inner.removeFromTop(10);
    int cx = dotsArea.getCentreX();
    m_d1.setBounds(cx - 12, dotsArea.getY(), 6, 6);
    m_d2.setBounds(cx, dotsArea.getY(), 6, 6);
    m_d3.setBounds(cx + 12, dotsArea.getY(), 6, 6);
}

#endif
