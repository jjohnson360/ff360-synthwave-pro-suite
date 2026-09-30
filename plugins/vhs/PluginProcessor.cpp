#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VHSPluginProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto makeFloatParam = [&](const juce::String& id, const juce::String& name, float min, float max, float def, const juce::String& unit = "%") {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ id, 1 }, name, juce::NormalisableRange<float>(min, max, 0.1f), def,
            juce::AudioParameterFloatAttributes().withLabel(unit)));
    };

    makeFloatParam("sat", "Tape Saturation", 0.0f, 100.0f, 25.0f);
    makeFloatParam("wow", "Wow", 0.0f, 100.0f, 15.0f);
    makeFloatParam("flutter", "Flutter", 0.0f, 100.0f, 10.0f);
    makeFloatParam("drift", "Pitch Drift", 0.0f, 100.0f, 15.0f);
    makeFloatParam("noise", "Noise", 0.0f, 100.0f, 10.0f);
    makeFloatParam("hiss", "Hiss", 0.0f, 100.0f, 15.0f);
    makeFloatParam("dropouts", "Dropouts", 0.0f, 100.0f, 0.0f);
    makeFloatParam("lofi", "Lo-Fi", 0.0f, 100.0f, 0.0f);
    makeFloatParam("bitcrush", "Bit Reduction", 0.0f, 100.0f, 0.0f);
    makeFloatParam("hfloss", "HF Loss", 0.0f, 100.0f, 20.0f);
    makeFloatParam("stereodrift", "Stereo Drift", 0.0f, 100.0f, 20.0f);
    makeFloatParam("warble", "Warble", 0.0f, 100.0f, 10.0f);

    makeFloatParam("degrade", "DEGRADE", 0.0f, 100.0f, 0.0f);
    makeFloatParam("inGain", "Input Gain", -24.0f, 24.0f, 0.0f, "dB");
    makeFloatParam("outGain", "Output Gain", -24.0f, 24.0f, 0.0f, "dB");
    makeFloatParam("mix", "Mix", 0.0f, 100.0f, 100.0f);

    // Oversampling around the engine (ff360_ui::Oversampler)
    ff360_ui::Oversampler::addParameter(params); // Off / 2x (default) / 4x

    // Output trim, auto gain and bypass (ff360::FF360_DSP_OutputStage)
    ff360_ui::output::addParameters(params, true, false);

    return { params.begin(), params.end() };
}

VHSPluginProcessor::VHSPluginProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_degradeMacro(ff360::FF360_DSP_MacroSystem::createVhsDegradeMacro()),
      m_history(*this, {}, { "bypass", "oversampling" }),
      m_presetManager(m_apvts, m_history, "VHS", ff360::getVhsPresets(), { "bypass", "oversampling" }) {
}

void VHSPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_baseRate = sampleRate;
    m_maxBlock = samplesPerBlock;
    m_outputStage.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
    m_oversampler.prepare(2, samplesPerBlock);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
}

void VHSPluginProcessor::applyOversampling(int order) {
    m_oversampler.setOrder(order);
    const double rate = m_baseRate * m_oversampler.getFactor();
    const size_t block = static_cast<size_t>(m_maxBlock * m_oversampler.getFactor());
    m_tapeEngine.prepare(rate, block);

    // The host compensates this; the dry path (bypass, auto gain) is delayed to match
    const int latency = m_oversampler.getLatencySamples();
    m_outputStage.setDryDelay(static_cast<size_t>(latency));
    setLatencySamples(latency);
}

void VHSPluginProcessor::handleAsyncUpdate() {
    // The Oversampling setting changed: re-prepare with the audio thread held off
    suspendProcessing(true);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
    suspendProcessing(false);
}

void VHSPluginProcessor::releaseResources() {
    m_tapeEngine.reset();
    m_meteringBridge.reset();
}

void VHSPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Oversampling setting changed: switch on the message thread (it re-prepares the engine)
    if (ff360_ui::Oversampler::readOrder(m_apvts) != m_oversampler.getOrder())
        triggerAsyncUpdate();

    // Keep the input for bypass and auto gain
    m_outputStage.captureDry(buffer.getReadPointer(0), buffer.getReadPointer(numChannels > 1 ? 1 : 0), static_cast<size_t>(numSamples));

    // Read and update parameters
    const float degradeVal = m_apvts.getRawParameterValue("degrade")->load() * 0.01f;
    m_degradeMacro.setMacroValue(degradeVal);

    ff360::TapeParameters params;
    params.saturation = m_apvts.getRawParameterValue("sat")->load() * 0.01f;
    params.wow = m_apvts.getRawParameterValue("wow")->load() * 0.01f;
    params.flutter = m_apvts.getRawParameterValue("flutter")->load() * 0.01f;
    params.pitchDrift = m_apvts.getRawParameterValue("drift")->load() * 0.01f;
    params.noise = m_apvts.getRawParameterValue("noise")->load() * 0.01f;
    params.hiss = m_apvts.getRawParameterValue("hiss")->load() * 0.01f;
    params.dropouts = m_apvts.getRawParameterValue("dropouts")->load() * 0.01f;
    params.loFi = m_apvts.getRawParameterValue("lofi")->load() * 0.01f;
    params.bitReduction = m_apvts.getRawParameterValue("bitcrush")->load() * 0.01f;
    params.highFrequencyLoss = m_apvts.getRawParameterValue("hfloss")->load() * 0.01f;
    params.stereoDrift = m_apvts.getRawParameterValue("stereodrift")->load() * 0.01f;
    params.warble = m_apvts.getRawParameterValue("warble")->load() * 0.01f;

    // Apply DEGRADE macro additions
    if (degradeVal > 0.001f) {
        m_degradeMacro.apply([&](uint32_t id, float val) {
            switch (static_cast<ff360::TapeParamId>(id)) {
                case ff360::TapeParamId::Saturation: params.saturation = std::max(params.saturation, val); break;
                case ff360::TapeParamId::Wow: params.wow = std::max(params.wow, val); break;
                case ff360::TapeParamId::Flutter: params.flutter = std::max(params.flutter, val); break;
                case ff360::TapeParamId::PitchDrift: params.pitchDrift = std::max(params.pitchDrift, val); break;
                case ff360::TapeParamId::Noise: params.noise = std::max(params.noise, val); break;
                case ff360::TapeParamId::Hiss: params.hiss = std::max(params.hiss, val); break;
                case ff360::TapeParamId::Dropouts: params.dropouts = std::max(params.dropouts, val); break;
                case ff360::TapeParamId::LoFi: params.loFi = std::max(params.loFi, val); break;
                case ff360::TapeParamId::BitReduction: params.bitReduction = std::max(params.bitReduction, val); break;
                case ff360::TapeParamId::HighFrequencyLoss: params.highFrequencyLoss = std::max(params.highFrequencyLoss, val); break;
                case ff360::TapeParamId::StereoDrift: params.stereoDrift = std::max(params.stereoDrift, val); break;
                case ff360::TapeParamId::Warble: params.warble = std::max(params.warble, val); break;
                default: break;
            }
        });
    }

    params.inputGainDb = m_apvts.getRawParameterValue("inGain")->load();
    params.outputGainDb = 0.0f; // "outGain" is the post-mix output trim, applied by m_outputStage
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_tapeEngine.setParameters(params);

    // Audio DSP processing
    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_oversampler.process(buffer, std::min(2, numChannels), [this](float* l, float* r, size_t n) {
        m_tapeEngine.processStereo(l, r, n);
    });

    // Post-Phase-10 precision metering
    m_outputStage.process(left, right, static_cast<size_t>(numSamples), ff360_ui::output::readSettings(m_apvts));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* VHSPluginProcessor::createEditor() {
    return new VHSPluginEditor(*this);
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int VHSPluginProcessor::getNumPrograms() { return 1; }
int VHSPluginProcessor::getCurrentProgram() { return 0; }
void VHSPluginProcessor::setCurrentProgram(int) {}
const juce::String VHSPluginProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void VHSPluginProcessor::changeProgramName(int, const juce::String&) {}

void VHSPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    copyXmlToBinary(*xml, destData);
}

void VHSPluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VHSPluginProcessor();
}

#endif
