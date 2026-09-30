#pragma once

#include "ff360/ModulationEngine.h"
#include "ff360/ParameterManager.h"
#include "ff360/MeteringBridge.h"
#include "Presets.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>
#include "ff360_ui/PresetManager.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360/DeltaTap.h"

class NeonChorusProcessor : public juce::AudioProcessor {
public:
    NeonChorusProcessor();
    ~NeonChorusProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FF360 Neon Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return m_apvts; }
    const ff360::FF360_DSP_OutputStage& getOutputStage() const { return m_outputStage; }
    juce::AudioProcessorParameter* getBypassParameter() const override { return m_apvts.getParameter(ff360_ui::output::bypassId); }
    ff360_ui::EditHistory& getHistory() { return m_history; }
    ff360_ui::PresetManager& getPresetManager() { return m_presetManager; }
    // Editor size as a scale of its design size (ff360_ui::EditorScaling), saved with the session
    float getEditorScale() const { return m_editorScale.load(); }
    void setEditorScale(float s) { m_editorScale.store(s); }
    ff360::FF360_DSP_MeteringBridge& getMeteringBridge() { return m_meteringBridge; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Tempo sync: the "syncdiv" note lengths, and the LFO rate one of them gives at a tempo
    static juce::StringArray syncDivisionNames() { return { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16" }; }
    static float syncedRateHz(int divisionIndex, double bpm);

private:
    juce::AudioProcessorValueTreeState m_apvts;
    ff360::FF360_DSP_ModulationEngine m_chorusEngine;
    ff360::FF360_DSP_ParameterManager m_paramManager;
    ff360::FF360_DSP_MeteringBridge m_meteringBridge;
    ff360::FF360_DSP_OutputStage m_outputStage; // auto gain, output trim, bypass
    ff360::FF360_DSP_DeltaTap m_deltaTap;       // delta listen, at the engine's rate

    // Undo/redo, A/B and the preset menu. Owned here (not by the editor) so they survive
    // closing the plugin window; declared after m_apvts, which they use.
    ff360_ui::EditHistory m_history;
    ff360_ui::PresetManager m_presetManager;
    std::atomic<float> m_editorScale { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonChorusProcessor)
};

#endif
