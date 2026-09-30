#include "PluginEditor.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)

NeonWidthEditor::NeonWidthEditor(NeonWidthProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p),
      m_workflowBar(p.getHistory(), p.getPresetManager()),
      m_outputStrip(p.getApvts(), p.getOutputStage())
{
    setLookAndFeel(&m_lookAndFeel);

    addAndMakeVisible(m_workflowBar);
    m_workflowBar.attachKeyboardShortcuts(*this);
    addAndMakeVisible(m_outputStrip);

    addAndMakeVisible(m_mainPanel);
    addAndMakeVisible(m_scope);

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
    ff360_ui::showValuePopup(m_bassMonoSlider, m_processor.getApvts(), "bassmono", this);
    
    m_bassMonoLabel.setText("BASS MONO", juce::dontSendNotification);
    m_bassMonoLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_bassMonoLabel.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    ff360_ui::bindValueLabel(m_bassMonoSlider, m_bassMonoValueLabel);
    m_bassMonoValueLabel.setFont(ff360_ui::brandFont(9.0f, juce::Font::bold));
    m_bassMonoValueLabel.setJustificationType(juce::Justification::centredRight);
    
    addAndMakeVisible(m_bassMonoSlider);
    addAndMakeVisible(m_bassMonoLabel);
    addAndMakeVisible(m_bassMonoValueLabel);


    // Tooltips: every control explains itself on hover (the editor test fails on any without one)
    m_knobs["haas"].slider.setTooltip("Haas: widens by delaying one side slightly.");
    m_knobs["microdelay"].slider.setTooltip("Micro Delay: delay between left and right, in ms.");
    m_knobs["detune"].slider.setTooltip("Detune: detunes left against right for width.");
    m_knobs["rotation"].slider.setTooltip("Rotation: rotates the stereo image left or right.");
    m_knobs["freqcrossover"].slider.setTooltip("Crossover: frequency above which Freq Width applies.");
    m_knobs["freqwidth"].slider.setTooltip("Freq Width: widens only above the crossover, keeping the low end focused.");
    m_knobs["mix"].slider.setTooltip("Mix: balance between the dry signal and the widened one.");
    m_faders["mswidth"].slider.setTooltip("Width: mid/side width. 0% is mono, 100% unchanged, 200% extra wide.");
    m_bassMonoSlider.setTooltip("Bass Mono: keeps everything below this frequency in mono (0 = off).");

    // Resizable (75% to 200%, aspect locked), reopening at the size it was left at
    ff360_ui::EditorScaling::setup(*this, m_processor.getEditorScale());
    startTimerHz(30);
}

NeonWidthEditor::~NeonWidthEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void NeonWidthEditor::timerCallback() {
    std::array<float, NeonWidthProcessor::kScopeBufferSize> left{}, right{};
    size_t count = 0;
    m_processor.getLatestScopeBuffers(left, right, count);

    const auto metrics = m_processor.getStereoEngine().getMetrics();
    m_scope.updateData(metrics.phaseCorrelation, metrics.balance, left.data(), right.data(), count);
}

void NeonWidthEditor::createKnob(const std::string& id, const juce::String& name) {
    auto& k = m_knobs[id];
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        m_processor.getApvts(), id, k.slider);
    ff360_ui::showValuePopup(k.slider, m_processor.getApvts(), id, this);
    
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(ff360_ui::brandFont(8.5f, juce::Font::plain));
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
    ff360_ui::showValuePopup(f.slider, m_processor.getApvts(), id, this);
    
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::centred);
    f.label.setFont(ff360_ui::brandFont(8.5f, juce::Font::plain));
    f.label.setColour(juce::Label::textColourId, juce::Colour(ff360_ui::Colors::TextDim));
    
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
}

void NeonWidthEditor::paint(juce::Graphics& g) {
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
    g.drawText("4 * STEREO IMAGING & MOVEMENT", 16, 12, bounds.getWidth() - 32, 12, juce::Justification::left);
    
    g.setFont(ff360_ui::brandFont(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(ff360_ui::Colors::TextDim));
    g.drawText("NEON WIDTH", 16, 26, bounds.getWidth() - 32, 14, juce::Justification::left);
}

void NeonWidthEditor::resized() {
    auto bounds = ff360_ui::EditorScaling::designBounds().reduced(16);
    bounds.removeFromTop(24);
    bounds.removeFromTop(4);
    m_workflowBar.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(6);
    m_outputStrip.setBounds(bounds.removeFromBottom(28));
    bounds.removeFromBottom(8);
    
    m_mainPanel.setBounds(bounds);
    auto inner = bounds.reduced(14);
    
    // Live goniometer / phase correlation scope
    m_scope.setBounds(inner.removeFromTop(120));
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

    // Laid out at the design size; scale everything to the window
    ff360_ui::EditorScaling::applyToChildren(*this);
    m_processor.setEditorScale(ff360_ui::EditorScaling::scaleOf(*this));
}

#endif
