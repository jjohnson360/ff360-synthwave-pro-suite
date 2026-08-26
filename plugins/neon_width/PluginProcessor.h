#pragma once

#include "ff360/StereoEngine.h"
#include "ff360/ParameterManager.h"
#include "ff360/MeteringBridge.h"
#include "Presets.h"
#include <array>

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>

class NeonWidthProcessor : public juce::AudioProcessor {
public:
    NeonWidthProcessor();
    ~NeonWidthProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FF360 Neon Width"; }
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
    ff360::FF360_DSP_StereoEngine& getStereoEngine() { return m_stereoEngine; }
    ff360::FF360_DSP_MeteringBridge& getMeteringBridge() { return m_meteringBridge; }

    // Thread-safe snapshot of the most recent block for the UI goniometer.
    // Safe to call from the message thread while processBlock() runs concurrently.
    static constexpr size_t kScopeBufferSize = 128;
    void getLatestScopeBuffers(std::array<float, kScopeBufferSize>& outL,
                                std::array<float, kScopeBufferSize>& outR,
                                size_t& outCount) const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState m_apvts;
    ff360::FF360_DSP_StereoEngine m_stereoEngine;
    ff360::FF360_DSP_ParameterManager m_paramManager;
    ff360::FF360_DSP_MeteringBridge m_meteringBridge;

    mutable juce::SpinLock m_scopeLock;
    std::array<float, kScopeBufferSize> m_latestL{};
    std::array<float, kScopeBufferSize> m_latestR{};
    size_t m_latestCount = 0;

    std::vector<ff360::Preset> m_presets;
    int m_currentPresetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonWidthProcessor)
};

#endif
