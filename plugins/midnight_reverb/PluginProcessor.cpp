#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MidnightReverbProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "alg", 1 }, "Algorithm",
        juce::StringArray{ "Digital Hall", "Dark Plate", "Gated Room", "Synth Room", "Endless", "Dream" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "decay", 1 }, "Decay Time", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "predelay", 1 }, "Pre-delay", juce::NormalisableRange<float>(0.0f, 250.0f, 0.1f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "synctoggle", 1 }, "Tempo Sync", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "ducking", 1 }, "Ducking", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "width", 1 }, "Width", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "modulation", 1 }, "Modulation", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "lowdamp", 1 }, "Low Cut", juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.4f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "highdamp", 1 }, "High Damping", juce::NormalisableRange<float>(1000.0f, 18000.0f, 1.0f, 0.4f), 8000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "freeze", 1 }, "Infinite Freeze", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

MidnightReverbProcessor::MidnightReverbProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getMidnightReverbPresets()) {
}

void MidnightReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_reverbEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
}

void MidnightReverbProcessor::releaseResources() {
    m_reverbEngine.reset();
    m_meteringBridge.reset();
}

void MidnightReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Read tempo if available from playhead
    float hostBpm = 120.0f;
    if (auto* playHead = getPlayHead()) {
        if (auto pos = playHead->getPosition()) {
            if (pos->getBpm()) hostBpm = static_cast<float>(*pos->getBpm());
        }
    }

    ff360::ReverbParameters params;
    params.algorithm = static_cast<ff360::ReverbAlgorithmType>(static_cast<int>(m_apvts.getRawParameterValue("alg")->load()));
    params.decayTime = m_apvts.getRawParameterValue("decay")->load() * 0.01f;
    params.preDelayMs = m_apvts.getRawParameterValue("predelay")->load();
    params.preDelayBpm = hostBpm;
    params.preDelaySync = (m_apvts.getRawParameterValue("synctoggle")->load() > 0.5f);
    params.ducking = m_apvts.getRawParameterValue("ducking")->load() * 0.01f;
    params.width = m_apvts.getRawParameterValue("width")->load() * 0.01f;
    params.modulation = m_apvts.getRawParameterValue("modulation")->load() * 0.01f;
    params.lowDampingHz = m_apvts.getRawParameterValue("lowdamp")->load();
    params.highDampingHz = m_apvts.getRawParameterValue("highdamp")->load();
    params.freeze = (m_apvts.getRawParameterValue("freeze")->load() > 0.5f);
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_reverbEngine.setParameters(params);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_reverbEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* MidnightReverbProcessor::createEditor() {
    return new MidnightReverbEditor(*this);
}

int MidnightReverbProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int MidnightReverbProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void MidnightReverbProcessor::setCurrentProgram(int index) {
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
const juce::String MidnightReverbProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void MidnightReverbProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void MidnightReverbProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MidnightReverbProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

#endif
