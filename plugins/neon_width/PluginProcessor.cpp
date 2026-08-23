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
      m_presets(ff360::getNeonWidthPresets()) {
}

void NeonWidthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_stereoEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
    m_latestL.assign(128, 0.0f);
    m_latestR.assign(128, 0.0f);
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

    // Capture small sample block for goniometer
    const size_t take = std::min(static_cast<size_t>(numSamples), static_cast<size_t>(128));
    m_latestL.resize(take);
    m_latestR.resize(take);
    std::copy(left, left + take, m_latestL.begin());
    std::copy(right, right + take, m_latestR.begin());
}

juce::AudioProcessorEditor* NeonWidthProcessor::createEditor() {
    return new NeonWidthEditor(*this);
}

int NeonWidthProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int NeonWidthProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void NeonWidthProcessor::setCurrentProgram(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) {
        m_currentPresetIndex = index;
        m_paramManager.importFromJson(m_presets[index].jsonContent);
        for (const auto& desc : m_paramManager.getDescriptors()) {
            if (auto* p = m_apvts.getParameter(desc.id)) {
                p->setValueNotifyingHost(m_paramManager.getNormalizedValue(desc.id));
            }
        }
    }
}
const juce::String NeonWidthProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void NeonWidthProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void NeonWidthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeonWidthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NeonWidthProcessor();
}

#endif
