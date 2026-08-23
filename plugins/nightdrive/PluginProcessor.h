#pragma once

#include "ff360/GenerativeEngine.h"
#include "ff360/GranularTexture.h"
#include "ff360/ModulationEngine.h"
#include "ff360/ReverbEngine.h"
#include "ff360/MacroSystem.h"
#include "ff360/ParameterManager.h"
#include "ff360/MeteringBridge.h"
#include "Presets.h"

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>

class NightDriveProcessor : public juce::AudioProcessor {
public:
    NightDriveProcessor();
    ~NightDriveProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FF360 NightDrive"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return m_apvts; }
    ff360::FF360_DSP_MeteringBridge& getMeteringBridge() { return m_meteringBridge; }
    bool isChordFlowActive() const noexcept { return m_chordFlowDetected; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState m_apvts;

    // Composed DSP modules
    ff360::FF360_DSP_ModulationEngine m_droneChorus;
    ff360::FF360_DSP_GranularTexture m_granularTexture;
    ff360::FF360_DSP_ReverbEngine m_reverbWash;
    ff360::FF360_DSP_MacroSystem m_evolveMacro;
    ff360::FF360_DSP_ParameterManager m_paramManager;
    ff360::FF360_DSP_MeteringBridge m_meteringBridge;

    // Filter sweep LFO
    float m_filterLfoPhase = 0.0f;
    ff360::BiquadFilter m_filterL;
    ff360::BiquadFilter m_filterR;

    // Arp synthesis
    float m_arpPhase = 0.0f;
    size_t m_arpStep = 0;
    size_t m_samplesUntilArpStep = 0;

    // Scale / Chord table
    bool m_chordFlowDetected = false;
    std::vector<int> m_currentScaleNotes;

    std::vector<ff360::Preset> m_presets;
    int m_currentPresetIndex = 0;

    void updateScaleNotes(int scaleIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NightDriveProcessor)
};

#endif
