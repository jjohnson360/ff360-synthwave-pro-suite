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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "scsensitivity", 1 }, "Sidechain Trigger", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Oversampling around the engine (ff360_ui::Oversampler)
    ff360_ui::Oversampler::addParameter(params); // Off / 2x (default) / 4x

    // Output trim, auto gain and bypass (ff360::FF360_DSP_OutputStage)
    ff360_ui::output::addParameters(params, true);
    ff360_ui::output::addDeltaParameter(params); // hear only what the effect adds

    return { params.begin(), params.end() };
}

CyberpunkGlitchProcessor::CyberpunkGlitchProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                     .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_history(*this, {}, { "bypass", "delta", "oversampling" }),
      m_presetManager(m_apvts, m_history, "Cyberpunk Glitch", ff360::getCyberpunkGlitchPresets(), { "bypass", "delta", "oversampling" }) {
}

bool CyberpunkGlitchProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    const auto scSet = layouts.getChannelSet(true, 1);
    return scSet.isDisabled() || scSet == juce::AudioChannelSet::mono() || scSet == juce::AudioChannelSet::stereo();
}

void CyberpunkGlitchProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_baseRate = sampleRate;
    m_maxBlock = samplesPerBlock;
    m_outputStage.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
    m_oversampler.prepare(2, samplesPerBlock);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
}

void CyberpunkGlitchProcessor::applyOversampling(int order) {
    m_oversampler.setOrder(order);
    const double rate = m_baseRate * m_oversampler.getFactor();
    const size_t block = static_cast<size_t>(m_maxBlock * m_oversampler.getFactor());
    m_glitchEngine.prepare(rate, block);
    m_scUpL.assign(block, 0.0f);
    m_scUpR.assign(block, 0.0f);

    // The host compensates this; the dry path (bypass, auto gain) is delayed to match
    m_deltaTap.prepare(rate, block);

    const int latency = m_oversampler.getLatencySamples();
    m_outputStage.setDryDelay(static_cast<size_t>(latency));
    setLatencySamples(latency);
}

void CyberpunkGlitchProcessor::handleAsyncUpdate() {
    // The Oversampling setting changed: re-prepare with the audio thread held off
    suspendProcessing(true);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
    suspendProcessing(false);
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

    // Oversampling setting changed: switch on the message thread (it re-prepares the engine)
    if (ff360_ui::Oversampler::readOrder(m_apvts) != m_oversampler.getOrder())
        triggerAsyncUpdate();

    // Keep the input for bypass and auto gain
    m_outputStage.captureDry(buffer.getReadPointer(0), buffer.getReadPointer(numChannels > 1 ? 1 : 0), static_cast<size_t>(numSamples));

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
    params.sidechainSensitivity = m_apvts.getRawParameterValue("scsensitivity")->load() * 0.01f;

    m_glitchEngine.setParameters(params);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    // Optional external sidechain: forces a fresh glitch slice on each incoming transient
    // (e.g. "glitch on the kick") when a sidechain is routed in and Sidechain Trigger > 0.
    auto scBuffer = getBusBuffer(buffer, true, 1);
    const float* scLeft = nullptr;
    const float* scRight = nullptr;
    if (scBuffer.getNumChannels() > 0) {
        scLeft = scBuffer.getReadPointer(0);
        scRight = (scBuffer.getNumChannels() > 1) ? scBuffer.getReadPointer(1) : scLeft;
    }

    const bool delta = ff360_ui::output::readDelta(m_apvts);
    m_oversampler.process(buffer, std::min(2, numChannels), [&](float* l, float* r, size_t n) {
        // The sidechain only drives transient detection, so holding each sample is enough
        const float* upL = scLeft;
        const float* upR = scRight;
        const size_t factor = static_cast<size_t>(m_oversampler.getFactor());
        if (scLeft != nullptr && factor > 1 && n <= m_scUpL.size()) {
            for (size_t i = 0; i < n; ++i) {
                m_scUpL[i] = scLeft[i / factor];
                m_scUpR[i] = scRight[i / factor];
            }
            upL = m_scUpL.data();
            upR = m_scUpR.data();
        }
        m_deltaTap.capture(l, r, n);
        m_glitchEngine.processStereo(l, r, n, upL, upR);
        m_deltaTap.apply(l, r, n, delta, 1.0f); // delta: the difference the glitches make
    });
    m_outputStage.process(left, right, static_cast<size_t>(numSamples), ff360_ui::output::readSettings(m_apvts));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* CyberpunkGlitchProcessor::createEditor() {
    return new CyberpunkGlitchEditor(*this);
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int CyberpunkGlitchProcessor::getNumPrograms() { return 1; }
int CyberpunkGlitchProcessor::getCurrentProgram() { return 0; }
void CyberpunkGlitchProcessor::setCurrentProgram(int) {}
const juce::String CyberpunkGlitchProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void CyberpunkGlitchProcessor::changeProgramName(int, const juce::String&) {}

void CyberpunkGlitchProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    copyXmlToBinary(*xml, destData);
}

void CyberpunkGlitchProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new CyberpunkGlitchProcessor();
}

#endif
