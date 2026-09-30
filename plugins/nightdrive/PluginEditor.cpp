#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

NightDriveEditor::NightDriveEditor(NightDriveProcessor& p)
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

    // Fader labels/order follow the UI mockup's ambience theme (Rain / Road / City / Engine / Neon / Atmos).
    // "Tape" from the mockup has no equivalent layer in this engine, so it's omitted rather than mislabeled.
    createFader("granularlevel", "RAIN");
    createFader("dronelevel", "ROAD");
    createFader("density", "CITY");
    createFader("filtermove", "ENGINE");
    createFader("arplevel", "NEON");
    createFader("reverbwash", "ATMOS");

    // Evolve horizontal slider
    m_evolveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_evolveSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_evolveSlider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::AccessibleSky));
    m_evolveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "evolve", m_evolveSlider);
    ff360_ui::showValuePopup(m_evolveSlider, m_processor.getApvts(), "evolve", this);
    
    m_evolveLabel.setText("EVOLUTION", juce::dontSendNotification);
    m_evolveLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_evolveLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    ff360_ui::bindValueLabel(m_evolveSlider, m_evolveValueLabel);
    m_evolveValueLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_evolveValueLabel.setJustificationType(juce::Justification::centredRight);
    
    addAndMakeVisible(m_evolveSlider);
    addAndMakeVisible(m_evolveLabel);
    addAndMakeVisible(m_evolveValueLabel);

    // Mix horizontal slider
    m_mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_mixSlider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::MetallicGold));
    m_mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "mix", m_mixSlider);
    ff360_ui::showValuePopup(m_mixSlider, m_processor.getApvts(), "mix", this);
    
    m_mixLabel.setText("MIX", juce::dontSendNotification);
    m_mixLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_mixLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    ff360_ui::bindValueLabel(m_mixSlider, m_mixValueLabel);
    m_mixValueLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_mixValueLabel.setJustificationType(juce::Justification::centredRight);
    
    addAndMakeVisible(m_mixSlider);
    addAndMakeVisible(m_mixLabel);
    addAndMakeVisible(m_mixValueLabel);

    // Sidechain duck slider
    m_scDuckSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_scDuckSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_scDuckSlider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::AccessibleSky));
    m_scDuckAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), "scduck", m_scDuckSlider);
    ff360_ui::showValuePopup(m_scDuckSlider, m_processor.getApvts(), "scduck", this);

    m_scDuckLabel.setText("SC DUCK", juce::dontSendNotification);
    m_scDuckLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_scDuckLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));

    ff360_ui::bindValueLabel(m_scDuckSlider, m_scDuckValueLabel);
    m_scDuckValueLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_scDuckValueLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(m_scDuckSlider);
    addAndMakeVisible(m_scDuckLabel);
    addAndMakeVisible(m_scDuckValueLabel);

    // Scale Lock
    m_scaleBox.addItem("Major", 1);
    m_scaleBox.addItem("Natural Minor", 2);
    m_scaleBox.addItem("Dorian", 3);
    m_scaleBox.addItem("Phrygian", 4);
    m_scaleBox.addItem("Lydian", 5);
    m_scaleBox.addItem("Mixolydian", 6);
    m_scaleBox.addItem("Synthwave Pentatonic", 7);
    m_scaleBox.addItem("ChordFlow (Auto)", 8);
    m_scaleBox.setJustificationType(juce::Justification::centred);
    m_scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        m_processor.getApvts(), "scalelock", m_scaleBox);
    addAndMakeVisible(m_scaleBox);


    // Tooltips: every control explains itself on hover (the editor test fails on any without one)
    m_faders["granularlevel"].slider.setTooltip("Rain: level of the granular rain texture.");
    m_faders["dronelevel"].slider.setTooltip("Road: level of the low drone.");
    m_faders["density"].slider.setTooltip("City: density of the rain texture, in grains per second.");
    m_faders["filtermove"].slider.setTooltip("Engine: how far the slow filter sweep moves.");
    m_faders["arplevel"].slider.setTooltip("Neon: level of the tempo-synced arpeggio.");
    m_faders["reverbwash"].slider.setTooltip("Atmos: size and amount of the reverb wash.");
    m_scaleBox.setTooltip("Scale / chord the drone and arpeggio notes are locked to.");
    m_evolveSlider.setTooltip("Evolution: animates the whole atmosphere (grain size, pitch spray, chorus and reverb movement).");
    m_mixSlider.setTooltip("Mix: level of the generated atmosphere.");
    m_scDuckSlider.setTooltip("SC Duck: pulls the atmosphere down while the sidechain input plays (needs a sidechain routed in).");

    // Resizable (75% to 200%, aspect locked), reopening at the size it was left at
    ff360_ui::EditorScaling::setup(*this, m_processor.getEditorScale());
}

NightDriveEditor::~NightDriveEditor() {
    setLookAndFeel(nullptr);
}

void NightDriveEditor::createFader(const std::string& id, const juce::String& name) {
    auto& f = m_faders[id];
    f.slider.setSliderStyle(juce::Slider::LinearVertical);
    f.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    f.slider.setColour(juce::Slider::thumbColourId, juce::Colour(ff360_ui::Colors::WarmAmberRed));
    f.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, f.slider);
    ff360_ui::showValuePopup(f.slider, m_processor.getApvts(), id, this);
    
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::centred);
    f.label.setFont(ff360_ui::brandFont(8.0f, juce::Font::bold));
    f.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
}

void NightDriveEditor::paint(juce::Graphics& g) {
    g.addTransform(ff360_ui::EditorScaling::transformFor(*this)); // draw at the design size
    auto bounds = ff360_ui::EditorScaling::designBounds().toFloat();
    
    // Background gradient
    juce::ColourGradient bgGrad(juce::Colour(ff360_ui::Colors::MatteCharcoal), 0, 0,
                                juce::Colour(0xFF131316), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();
    
    // Outer border
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold).withAlpha(0.16f));
    g.drawRect(bounds, 1.0f);
    
    // Eyebrow and Title
    g.setFont(ff360_ui::brandFont(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::MetallicGold));
    g.drawText("6 * GENERATIVE ATMOSPHERE", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(ff360_ui::brandFont(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("NIGHTDRIVE", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void NightDriveEditor::resized() {
    auto bounds = ff360_ui::EditorScaling::designBounds().reduced(16);
    bounds.removeFromTop(24);
    bounds.removeFromTop(4);
    m_workflowBar.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(6);
    m_outputStrip.setBounds(bounds.removeFromBottom(28));
    bounds.removeFromBottom(8);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Scene (tall)
    m_scene.setBounds(inner.removeFromTop(180));
    inner.removeFromTop(16);
    
    // Faders
    auto fadersArea = inner.removeFromTop(150);
    int faderW = 20;
    int faderCount = 6;
    int spacing = (fadersArea.getWidth() - (faderCount * faderW)) / (faderCount - 1);
    
    const char* fNames[] = {"granularlevel", "dronelevel", "density", "filtermove", "arplevel", "reverbwash"};
    for (int i = 0; i < faderCount; ++i) {
        auto& f = m_faders[fNames[i]];
        int x = fadersArea.getX() + i * (faderW + spacing);
        f.slider.setBounds(x, fadersArea.getY(), faderW, 130);
        f.label.setBounds(x - 20, f.slider.getBottom() + 4, faderW + 40, 14);
    }
    
    inner.removeFromTop(16);
    
    // Scale box
    m_scaleBox.setBounds(inner.removeFromTop(30));
    inner.removeFromTop(16);
    
    // Evolution slider
    auto evRow = inner.removeFromTop(20);
    m_evolveLabel.setBounds(evRow.getX(), evRow.getY(), 60, 12);
    m_evolveValueLabel.setBounds(evRow.getRight() - 50, evRow.getY(), 50, 12);
    m_evolveSlider.setBounds(evRow.getX(), evRow.getY() + 14, evRow.getWidth(), 6);
    
    inner.removeFromTop(16);
    
    // Mix slider
    auto mixRow = inner.removeFromTop(20);
    m_mixLabel.setBounds(mixRow.getX(), mixRow.getY(), 60, 12);
    m_mixValueLabel.setBounds(mixRow.getRight() - 50, mixRow.getY(), 50, 12);
    m_mixSlider.setBounds(mixRow.getX(), mixRow.getY() + 14, mixRow.getWidth(), 6);

    inner.removeFromTop(16);

    // Sidechain duck slider
    auto scRow = inner.removeFromTop(20);
    m_scDuckLabel.setBounds(scRow.getX(), scRow.getY(), 60, 12);
    m_scDuckValueLabel.setBounds(scRow.getRight() - 50, scRow.getY(), 50, 12);
    m_scDuckSlider.setBounds(scRow.getX(), scRow.getY() + 14, scRow.getWidth(), 6);

    // Laid out at the design size; scale everything to the window
    ff360_ui::EditorScaling::applyToChildren(*this);
    m_processor.setEditorScale(ff360_ui::EditorScaling::scaleOf(*this));
}

#endif
