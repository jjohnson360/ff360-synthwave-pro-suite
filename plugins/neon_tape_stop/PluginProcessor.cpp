#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NeonTapeStopProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "trigger", 1 }, "Stop Trigger", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "slowdown", 1 }, "Slowdown Time", juce::NormalisableRange<float>(0.05f, 4.0f, 0.01f, 0.5f), 0.8f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "pitchcurve", 1 }, "Pitch Curve", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "filtermove", 1 }, "Filter Sweep", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "recovery", 1 }, "Recovery Time", juce::NormalisableRange<float>(0.05f, 2.0f, 0.01f, 0.5f), 0.3f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "profile", 1 }, "Stop Profile",
        juce::StringArray{ "Vinyl Stop", "Tape Stop", "Digital Stop" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "reverse", 1 }, "Reverse Recovery", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

NeonTapeStopProcessor::NeonTapeStopProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getNeonTapeStopPresets()) {
}

void NeonTapeStopProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_tapeEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_controller.prepare(sampleRate);
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
}

void NeonTapeStopProcessor::releaseResources() {
    m_tapeEngine.reset();
    m_controller.reset();
    m_meteringBridge.reset();
}

void NeonTapeStopProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Check MIDI triggers (NoteOn triggers stop, NoteOff triggers recovery)
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            m_controller.triggerStop();
        } else if (msg.isNoteOff()) {
            m_controller.triggerRecovery();
        }
    }

    // Manual button sync
    const bool isTriggered = m_apvts.getRawParameterValue("trigger")->load() > 0.5f;
    if (isTriggered && m_controller.getState() == ff360::TapeStopState::Idle) {
        m_controller.triggerStop();
    } else if (!isTriggered && (m_controller.getState() == ff360::TapeStopState::Stopping || m_controller.getState() == ff360::TapeStopState::Stopped)) {
        m_controller.triggerRecovery();
    }

    ff360::TapeStopParameters p;
    p.slowdownTimeSec = m_apvts.getRawParameterValue("slowdown")->load();
    p.pitchCurve = m_apvts.getRawParameterValue("pitchcurve")->load() * 0.01f;
    p.filterMovement = m_apvts.getRawParameterValue("filtermove")->load() * 0.01f;
    p.recoveryTimeSec = m_apvts.getRawParameterValue("recovery")->load();
    p.reverseRecovery = (m_apvts.getRawParameterValue("reverse")->load() > 0.5f);
    p.profile = static_cast<ff360::TapeStopProfile>(static_cast<int>(m_apvts.getRawParameterValue("profile")->load()));
    m_controller.setParameters(p);

    // Update state machine and apply parameter changes to TapeEngine
    m_controller.updateAndApply(m_tapeEngine, static_cast<size_t>(numSamples));

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_tapeEngine.processStereo(left, right, static_cast<size_t>(numSamples));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* NeonTapeStopProcessor::createEditor() {
    return new NeonTapeStopEditor(*this);
}

int NeonTapeStopProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int NeonTapeStopProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void NeonTapeStopProcessor::setCurrentProgram(int index) {
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
const juce::String NeonTapeStopProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void NeonTapeStopProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void NeonTapeStopProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeonTapeStopProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NeonTapeStopProcessor();
}

#endif
