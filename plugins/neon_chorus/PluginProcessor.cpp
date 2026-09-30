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

    // Output trim, auto gain and bypass (ff360::FF360_DSP_OutputStage)
    ff360_ui::output::addParameters(params, true);

    return { params.begin(), params.end() };
}

NeonChorusProcessor::NeonChorusProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_history(*this, {}, { "bypass" }),
      m_presetManager(m_apvts, m_history, "Neon Chorus", ff360::getNeonChorusPresets(), { "bypass" }) {
}

void NeonChorusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_outputStage.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
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

    // Keep the input for bypass and auto gain
    m_outputStage.captureDry(buffer.getReadPointer(0), buffer.getReadPointer(numChannels > 1 ? 1 : 0), static_cast<size_t>(numSamples));

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
    m_outputStage.process(left, right, static_cast<size_t>(numSamples), ff360_ui::output::readSettings(m_apvts));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* NeonChorusProcessor::createEditor() {
    return new NeonChorusEditor(*this);
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int NeonChorusProcessor::getNumPrograms() { return 1; }
int NeonChorusProcessor::getCurrentProgram() { return 0; }
void NeonChorusProcessor::setCurrentProgram(int) {}
const juce::String NeonChorusProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void NeonChorusProcessor::changeProgramName(int, const juce::String&) {}

void NeonChorusProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    copyXmlToBinary(*xml, destData);
}

void NeonChorusProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NeonChorusProcessor();
}

#endif
