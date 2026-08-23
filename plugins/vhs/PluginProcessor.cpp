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

    return { params.begin(), params.end() };
}

VHSPluginProcessor::VHSPluginProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_degradeMacro(ff360::FF360_DSP_MacroSystem::createVhsDegradeMacro()),
      m_presets(ff360::getVhsPresets()) {
}

void VHSPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_tapeEngine.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);
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
    params.outputGainDb = m_apvts.getRawParameterValue("outGain")->load();
    params.mix = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    m_tapeEngine.setParameters(params);

    // Audio DSP processing
    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    m_tapeEngine.processStereo(left, right, static_cast<size_t>(numSamples));

    // Post-Phase-10 precision metering
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* VHSPluginProcessor::createEditor() {
    return new VHSPluginEditor(*this);
}

int VHSPluginProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int VHSPluginProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void VHSPluginProcessor::setCurrentProgram(int index) {
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
const juce::String VHSPluginProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) {
        return m_presets[index].name;
    }
    return {};
}
void VHSPluginProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) {
        m_presets[index].name = newName.toStdString();
    }
}

void VHSPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VHSPluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VHSPluginProcessor();
}

#endif
