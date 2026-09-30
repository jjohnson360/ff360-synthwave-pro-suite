#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NeonWidthProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "microdelay", 1 }, "Micro-delay", juce::NormalisableRange<float>(0.0f, 25.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "haas", 1 }, "Haas Widening", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "detune", 1 }, "Stereo Detune", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mswidth", 1 }, "M/S Width", juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "freqwidth", 1 }, "Freq-Dependent Width", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "freqcrossover", 1 }, "Freq Crossover", juce::NormalisableRange<float>(100.0f, 2000.0f, 1.0f, 0.5f), 500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "rotation", 1 }, "Stereo Rotation", juce::NormalisableRange<float>(-90.0f, 90.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("deg")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "bassmono", 1 }, "Bass Mono", juce::NormalisableRange<float>(0.0f, 400.0f, 1.0f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

NeonWidthProcessor::NeonWidthProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_history(*this, {}, {}),
      m_presetManager(m_apvts, m_history, "Neon Width", ff360::getNeonWidthPresets(), {}) {
}

void NeonWidthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_stereoEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);

    const juce::SpinLock::ScopedLockType sl(m_scopeLock);
    m_latestL.fill(0.0f);
    m_latestR.fill(0.0f);
    m_latestCount = 0;
}

void NeonWidthProcessor::releaseResources() {
    m_stereoEngine.reset();
    m_meteringBridge.reset();
}

void NeonWidthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    ff360::StereoParameters params;
    params.microDelayMs = m_apvts.getRawParameterValue("microdelay")->load();
    params.haasWidth = m_apvts.getRawParameterValue("haas")->load() * 0.01f;
    params.stereoDetune = m_apvts.getRawParameterValue("detune")->load() * 0.01f;
    params.msWidth = m_apvts.getRawParameterValue("mswidth")->load() * 0.01f;
    params.freqWidth = m_apvts.getRawParameterValue("freqwidth")->load() * 0.01f;
    params.freqWidthCrossoverHz = m_apvts.getRawParameterValue("freqcrossover")->load();
    params.stereoRotationDeg = m_apvts.getRawParameterValue("rotation")->load();
    params.bassMonoCutoffHz = m_apvts.getRawParameterValue("bassmono")->load();
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_stereoEngine.setParameters(params);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_stereoEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));

    // Capture a small sample block for the goniometer, under a spinlock so the
    // message thread can read a consistent snapshot without a torn/reallocating read.
    {
        const size_t take = std::min(static_cast<size_t>(numSamples), kScopeBufferSize);
        const juce::SpinLock::ScopedLockType sl(m_scopeLock);
        std::copy(left, left + take, m_latestL.begin());
        std::copy(right, right + take, m_latestR.begin());
        m_latestCount = take;
    }
}

juce::AudioProcessorEditor* NeonWidthProcessor::createEditor() {
    return new NeonWidthEditor(*this);
}

void NeonWidthProcessor::getLatestScopeBuffers(std::array<float, kScopeBufferSize>& outL,
                                                std::array<float, kScopeBufferSize>& outR,
                                                size_t& outCount) const noexcept {
    const juce::SpinLock::ScopedLockType sl(m_scopeLock);
    outCount = m_latestCount;
    std::copy(m_latestL.begin(), m_latestL.begin() + static_cast<long>(outCount), outL.begin());
    std::copy(m_latestR.begin(), m_latestR.begin() + static_cast<long>(outCount), outR.begin());
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int NeonWidthProcessor::getNumPrograms() { return 1; }
int NeonWidthProcessor::getCurrentProgram() { return 0; }
void NeonWidthProcessor::setCurrentProgram(int) {}
const juce::String NeonWidthProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void NeonWidthProcessor::changeProgramName(int, const juce::String&) {}

void NeonWidthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    copyXmlToBinary(*xml, destData);
}

void NeonWidthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NeonWidthProcessor();
}

#endif
