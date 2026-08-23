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

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ "seed", 1 }, "Recall Seed", 1, 99999, 42));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

RetroFXProcessor::RetroFXProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getRetroFXPresets()) {
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
    m_genEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
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

    // Check MIDI triggers
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            triggerGenerate();
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
    m_genEngine.selectGenerator(genId);

    ff360::GenerativeParameters p;
    p.syncMode = static_cast<ff360::GenerativeSyncMode>(static_cast<int>(m_apvts.getRawParameterValue("sync")->load()));
    p.freeDurationSec = 2.0f;
    p.hostBpm = currentBpm;
    p.intensity = m_apvts.getRawParameterValue("intensity")->load() * 0.01f;
    p.seed = static_cast<uint32_t>(m_apvts.getRawParameterValue("seed")->load());
    p.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_genEngine.setParameters(p);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_genEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* RetroFXProcessor::createEditor() {
    return new RetroFXEditor(*this);
}

int RetroFXProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int RetroFXProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void RetroFXProcessor::setCurrentProgram(int index) {
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
const juce::String RetroFXProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void RetroFXProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void RetroFXProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RetroFXProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new RetroFXProcessor();
}

#endif
