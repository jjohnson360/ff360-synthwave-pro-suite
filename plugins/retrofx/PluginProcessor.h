#pragma once

#include "ff360/GenerativeEngine.h"
#include "ff360/RetroGenerators.h"
#include "ff360/ParameterManager.h"
#include "ff360/MeteringBridge.h"
#include "Presets.h"
#include <atomic>

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>
#include "ff360_ui/PresetManager.h"
#include "ff360_ui/OutputStrip.h"
#include "ff360_ui/Oversampler.h"

class RetroFXProcessor : public juce::AudioProcessor,
                        private juce::AsyncUpdater {
public:
    RetroFXProcessor();
    ~RetroFXProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FF360 RetroFX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

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
    ff360::FF360_DSP_GenerativeEngine& getGenEngine() { return m_genEngine; }
    ff360::FF360_DSP_MeteringBridge& getMeteringBridge() { return m_meteringBridge; }

    void triggerGenerate();
    // From the UI: generates at the start of the next audio block (the generator isn't thread-safe)
    void requestGenerate() { m_generateRequested.store(true); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState m_apvts;
    ff360::FF360_DSP_GenerativeEngine m_genEngine;
    std::atomic<bool> m_generateRequested { false };
    int m_selectedGenerator = -1; // generator currently selected in m_genEngine
    ff360::FF360_DSP_ParameterManager m_paramManager;
    ff360::FF360_DSP_MeteringBridge m_meteringBridge;
    ff360::FF360_DSP_OutputStage m_outputStage; // auto gain, output trim, bypass
    ff360_ui::Oversampler m_oversampler;        // runs the engine at 2x / 4x
    double m_baseRate = 44100.0;
    int m_maxBlock = 512;

    // Re-prepares the engine at the oversampled rate and reports the new latency.
    // Only while audio isn't running: from prepareToPlay, or suspended (handleAsyncUpdate).
    void applyOversampling(int order);
    void handleAsyncUpdate() override;

    // Undo/redo, A/B and the preset menu. Owned here (not by the editor) so they survive
    // closing the plugin window; declared after m_apvts, which they use.
    ff360_ui::EditHistory m_history;
    ff360_ui::PresetManager m_presetManager;
    std::atomic<float> m_editorScale { 1.0f };

    void setupGenerators();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroFXProcessor)
};

#endif
