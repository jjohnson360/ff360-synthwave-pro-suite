#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout CyberpunkGlitchProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "division", 1 }, "Timing Division",
        juce::StringArray{ "1/4", "1/8", "1/16", "1/32" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "probability", 1 }, "Glitch Probability", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "reverse", 1 }, "Reverse Slices", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "freeze", 1 }, "Buffer Freeze", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "pitch", 1 }, "Pitch Shift", juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "bitcrush", 1 }, "Bitcrush", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "gate", 1 }, "Gate Length", juce::NormalisableRange<float>(10.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "filter", 1 }, "Filter Cutoff", juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 16000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "resonance", 1 }, "Resonance", juce::NormalisableRange<float>(0.5f, 4.0f, 0.01f), 0.707f,
        juce::AudioParameterFloatAttributes().withLabel("Q")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

CyberpunkGlitchProcessor::CyberpunkGlitchProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getCyberpunkGlitchPresets()) {
}

void CyberpunkGlitchProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_glitchEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
}

void CyberpunkGlitchProcessor::releaseResources() {
    m_glitchEngine.reset();
    m_meteringBridge.reset();
}

void CyberpunkGlitchProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Read host tempo if available
    float currentBpm = 120.0f;
    if (auto* playHead = getPlayHead()) {
        if (auto posOpt = playHead->getPosition()) {
            if (posOpt->getBpm().hasValue()) {
                currentBpm = static_cast<float>(*posOpt->getBpm());
            }
        }
    }

    ff360::GlitchParameters params;
    params.division = static_cast<ff360::GlitchTimingDivision>(static_cast<int>(m_apvts.getRawParameterValue("division")->load()));
    params.hostBpm = currentBpm;
    params.probability = m_apvts.getRawParameterValue("probability")->load() * 0.01f;
    params.reverse = (m_apvts.getRawParameterValue("reverse")->load() > 0.5f);
    params.freeze = (m_apvts.getRawParameterValue("freeze")->load() > 0.5f);
    params.pitchShiftSemitones = m_apvts.getRawParameterValue("pitch")->load();
    params.bitcrush = m_apvts.getRawParameterValue("bitcrush")->load() * 0.01f;
    params.gate = m_apvts.getRawParameterValue("gate")->load() * 0.01f;
    params.filterCutoffHz = m_apvts.getRawParameterValue("filter")->load();
    params.filterResonance = m_apvts.getRawParameterValue("resonance")->load();
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_glitchEngine.setParameters(params);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_glitchEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* CyberpunkGlitchProcessor::createEditor() {
    return new CyberpunkGlitchEditor(*this);
}

int CyberpunkGlitchProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int CyberpunkGlitchProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void CyberpunkGlitchProcessor::setCurrentProgram(int index) {
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
const juce::String CyberpunkGlitchProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void CyberpunkGlitchProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void CyberpunkGlitchProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CyberpunkGlitchProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new CyberpunkGlitchProcessor();
}

#endif
