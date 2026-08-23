#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NeonChorusProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "rate", 1 }, "Rate", juce::NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f), 0.8f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "depth", 1 }, "Depth", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "width", 1 }, "Width", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "detune", 1 }, "Detune", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "feedback", 1 }, "Feedback", juce::NormalisableRange<float>(0.0f, 95.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "predelay", 1 }, "Pre-delay", juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), 4.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "bassmono", 1 }, "Bass Mono", juce::NormalisableRange<float>(0.0f, 300.0f, 1.0f), 140.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "vintage", 1 }, "Vintage Character", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "quad", 1 }, "Quad Chorus (4-Voice)", true));

    return { params.begin(), params.end() };
}

NeonChorusProcessor::NeonChorusProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getNeonChorusPresets()) {
}

void NeonChorusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_chorusEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
}

void NeonChorusProcessor::releaseResources() {
    m_chorusEngine.reset();
    m_meteringBridge.reset();
}

void NeonChorusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    ff360::ModulationParameters params;
    params.rateHz = m_apvts.getRawParameterValue("rate")->load();
    params.depth = m_apvts.getRawParameterValue("depth")->load() * 0.01f;
    params.width = m_apvts.getRawParameterValue("width")->load() * 0.01f;
    params.detune = m_apvts.getRawParameterValue("detune")->load() * 0.01f;
    params.feedback = m_apvts.getRawParameterValue("feedback")->load() * 0.01f;
    params.preDelayMs = m_apvts.getRawParameterValue("predelay")->load();
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;
    params.bassMonoCrossoverHz = m_apvts.getRawParameterValue("bassmono")->load();
    params.character = (m_apvts.getRawParameterValue("vintage")->load() > 0.5f)
                       ? ff360::ChorusCharacter::Vintage : ff360::ChorusCharacter::Modern;
    params.mode = (m_apvts.getRawParameterValue("quad")->load() > 0.5f)
                  ? ff360::ChorusMode::Quad4Voice : ff360::ChorusMode::Stereo2Voice;

    m_chorusEngine.setParameters(params);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_chorusEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* NeonChorusProcessor::createEditor() {
    return new NeonChorusEditor(*this);
}

int NeonChorusProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int NeonChorusProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void NeonChorusProcessor::setCurrentProgram(int index) {
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
const juce::String NeonChorusProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void NeonChorusProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void NeonChorusProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeonChorusProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

#endif
