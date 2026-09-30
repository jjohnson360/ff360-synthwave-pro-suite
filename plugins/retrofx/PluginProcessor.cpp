#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RetroFXProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "generator", 1 }, "Generator Type",
        juce::StringArray{
            "Noise Sweep", "Pitch Sweep", "Laser", "Reverse",
            "Impact", "Riser", "Downlifter", "Digital Sweep", "Tape Sweep"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "sync", 1 }, "Duration / Sync",
        juce::StringArray{ "Free (2s)", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 3));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "intensity", 1 }, "Generate Intensity", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 85.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "startpitch", 1 }, "Start Pitch", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), -12.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "endpitch", 1 }, "End Pitch", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 12.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "length", 1 }, "Free Length", juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f, 0.5f), 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ "seed", 1 }, "Recall Seed", 1, 99999, 42));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Oversampling around the engine (ff360_ui::Oversampler)
    ff360_ui::Oversampler::addParameter(params); // Off / 2x (default) / 4x

    // Output trim, bypass (ff360::FF360_DSP_OutputStage)
    ff360_ui::output::addParameters(params, false);

    return { params.begin(), params.end() };
}

RetroFXProcessor::RetroFXProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_history(*this, {}, { "bypass", "oversampling" }),
      m_presetManager(m_apvts, m_history, "RetroFX", ff360::getRetroFXPresets(), { "bypass", "oversampling" }) {
    setupGenerators();
}

void RetroFXProcessor::setupGenerators() {
    m_genEngine.registerGenerator(0, std::make_shared<ff360::NoiseSweepGenerator>());
    m_genEngine.registerGenerator(1, std::make_shared<ff360::PitchSweepGenerator>());
    m_genEngine.registerGenerator(2, std::make_shared<ff360::LaserGenerator>());
    m_genEngine.registerGenerator(3, std::make_shared<ff360::ReverseGenerator>());
    m_genEngine.registerGenerator(4, std::make_shared<ff360::ImpactGenerator>());
    m_genEngine.registerGenerator(5, std::make_shared<ff360::RiserGenerator>());
    m_genEngine.registerGenerator(6, std::make_shared<ff360::DownlifterGenerator>());
    m_genEngine.registerGenerator(7, std::make_shared<ff360::DigitalSweepGenerator>());
    m_genEngine.registerGenerator(8, std::make_shared<ff360::TapeSweepGenerator>());
}

void RetroFXProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_baseRate = sampleRate;
    m_maxBlock = samplesPerBlock;
    m_outputStage.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
    m_oversampler.prepare(2, samplesPerBlock);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
}

void RetroFXProcessor::applyOversampling(int order) {
    m_oversampler.setOrder(order);
    const double rate = m_baseRate * m_oversampler.getFactor();
    const size_t block = static_cast<size_t>(m_maxBlock * m_oversampler.getFactor());
    m_genEngine.prepare(rate, block);
    m_selectedGenerator = -1; // re-select at the new rate

    // The host compensates this; the dry path (bypass, auto gain) is delayed to match
    const int latency = m_oversampler.getLatencySamples();
    m_outputStage.setDryDelay(static_cast<size_t>(latency));
    setLatencySamples(latency);
}

void RetroFXProcessor::handleAsyncUpdate() {
    // The Oversampling setting changed: re-prepare with the audio thread held off
    suspendProcessing(true);
    applyOversampling(ff360_ui::Oversampler::readOrder(m_apvts));
    suspendProcessing(false);
}

void RetroFXProcessor::releaseResources() {
    m_genEngine.reset();
    m_meteringBridge.reset();
}

void RetroFXProcessor::triggerGenerate() {
    const uint32_t seed = static_cast<uint32_t>(m_apvts.getRawParameterValue("seed")->load());
    m_genEngine.trigger(seed);
}

void RetroFXProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Oversampling setting changed: switch on the message thread (it re-prepares the engine)
    if (ff360_ui::Oversampler::readOrder(m_apvts) != m_oversampler.getOrder())
        triggerAsyncUpdate();

    // Keep the input for bypass and auto gain
    m_outputStage.captureDry(buffer.getReadPointer(0), buffer.getReadPointer(numChannels > 1 ? 1 : 0), static_cast<size_t>(numSamples));

    // GENERATE FX button or a MIDI note; fired below, once this block's settings are applied
    bool generate = m_generateRequested.exchange(false);
    for (const auto metadata : midiMessages) {
        if (metadata.getMessage().isNoteOn()) {
            generate = true;
        }
    }

    // Read host tempo
    float currentBpm = 120.0f;
    if (auto* playHead = getPlayHead()) {
        if (auto posOpt = playHead->getPosition()) {
            if (posOpt->getBpm().hasValue()) {
                currentBpm = static_cast<float>(*posOpt->getBpm());
            }
        }
    }

    const int genId = static_cast<int>(m_apvts.getRawParameterValue("generator")->load());
    // Only on a change: selecting resets the generator's oscillator and filters, which clicks
    if (genId != m_selectedGenerator) {
        m_genEngine.selectGenerator(genId);
        m_selectedGenerator = genId;
    }

    ff360::GenerativeParameters p;
    p.syncMode = static_cast<ff360::GenerativeSyncMode>(static_cast<int>(m_apvts.getRawParameterValue("sync")->load()));
    p.freeDurationSec = m_apvts.getRawParameterValue("length")->load();
    p.hostBpm = currentBpm;
    p.intensity = m_apvts.getRawParameterValue("intensity")->load() * 0.01f;
    p.startPitchSemitones = m_apvts.getRawParameterValue("startpitch")->load();
    p.endPitchSemitones = m_apvts.getRawParameterValue("endpitch")->load();
    p.seed = static_cast<uint32_t>(m_apvts.getRawParameterValue("seed")->load());
    p.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_genEngine.setParameters(p);

    if (generate)
        triggerGenerate();

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_oversampler.process(buffer, std::min(2, numChannels), [this](float* l, float* r, size_t n) {
        m_genEngine.processStereo(l, r, n);
    });
    m_outputStage.process(left, right, static_cast<size_t>(numSamples), ff360_ui::output::readSettings(m_apvts));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* RetroFXProcessor::createEditor() {
    return new RetroFXEditor(*this);
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int RetroFXProcessor::getNumPrograms() { return 1; }
int RetroFXProcessor::getCurrentProgram() { return 0; }
void RetroFXProcessor::setCurrentProgram(int) {}
const juce::String RetroFXProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void RetroFXProcessor::changeProgramName(int, const juce::String&) {}

void RetroFXProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    copyXmlToBinary(*xml, destData);
}

void RetroFXProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new RetroFXProcessor();
}

#endif
