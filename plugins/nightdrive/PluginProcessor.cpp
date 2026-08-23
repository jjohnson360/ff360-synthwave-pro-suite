#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NightDriveProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "dronelevel", 1 }, "Drone Level", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "granularlevel", 1 }, "Granular Texture", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "arplevel", 1 }, "Arp Motion", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "evolve", 1 }, "Evolve Macro", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "density", 1 }, "Grain Density", juce::NormalisableRange<float>(5.0f, 60.0f, 0.1f), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel("gr/s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "filtermove", 1 }, "Filter Movement", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "reverbwash", 1 }, "Reverb Wash", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 65.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "scalelock", 1 }, "Scale / Chord Lock",
        juce::StringArray{
            "Major", "Natural Minor", "Dorian", "Phrygian",
            "Lydian", "Mixolydian", "Synthwave Pentatonic", "ChordFlow (Auto)"
        }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

NightDriveProcessor::NightDriveProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_presets(ff360::getNightDrivePresets()) {
    updateScaleNotes(1); // Natural Minor default
}

void NightDriveProcessor::updateScaleNotes(int scaleIndex) {
    // Pitch semitones from root (A): 0 = A, 2 = B, 3 = C, etc.
    switch (scaleIndex) {
        case 0: m_currentScaleNotes = { 0, 2, 4, 5, 7, 9, 11 }; break; // Major
        case 1: m_currentScaleNotes = { 0, 2, 3, 5, 7, 8, 10 }; break; // Natural Minor
        case 2: m_currentScaleNotes = { 0, 2, 3, 5, 7, 9, 10 }; break; // Dorian
        case 3: m_currentScaleNotes = { 0, 1, 3, 5, 7, 8, 10 }; break; // Phrygian
        case 4: m_currentScaleNotes = { 0, 2, 4, 6, 7, 9, 11 }; break; // Lydian
        case 5: m_currentScaleNotes = { 0, 2, 4, 5, 7, 9, 10 }; break; // Mixolydian
        case 6: m_currentScaleNotes = { 0, 3, 5, 7, 10 };       break; // Synthwave Pentatonic
        case 7: // ChordFlow soft-dependency / auto
            if (m_chordFlowDetected) {
                // ChordFlow live scale
            } else {
                m_currentScaleNotes = { 0, 2, 3, 5, 7, 8, 10 }; // Fallback to minor
            }
            break;
        default: m_currentScaleNotes = { 0, 2, 3, 5, 7, 8, 10 }; break;
    }
}

void NightDriveProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_droneChorus.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_granularTexture.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_reverbWash.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);

    m_filterL.configure(ff360::BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 3000.0f, 1.5f);
    m_filterR.configure(ff360::BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 3000.0f, 1.5f);

    m_arpPhase = 0.0f;
    m_arpStep = 0;
    m_samplesUntilArpStep = static_cast<size_t>(sampleRate * 0.25); // 1/16th note at 120bpm
}

void NightDriveProcessor::releaseResources() {
    m_droneChorus.reset();
    m_granularTexture.reset();
    m_reverbWash.reset();
    m_meteringBridge.reset();
}

void NightDriveProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0) return;

    // Detect ChordFlow MIDI / Chord messages
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            m_chordFlowDetected = true;
        }
    }

    const float evolveNorm = m_apvts.getRawParameterValue("evolve")->load() * 0.01f;
    const float droneLvl = m_apvts.getRawParameterValue("dronelevel")->load() * 0.01f;
    const float granLvl = m_apvts.getRawParameterValue("granularlevel")->load() * 0.01f;
    const float arpLvl = m_apvts.getRawParameterValue("arplevel")->load() * 0.01f;
    const float densityVal = m_apvts.getRawParameterValue("density")->load();
    const float filterMove = m_apvts.getRawParameterValue("filtermove")->load() * 0.01f;
    const float reverbWash = m_apvts.getRawParameterValue("reverbwash")->load() * 0.01f;
    const int scaleIdx = static_cast<int>(m_apvts.getRawParameterValue("scalelock")->load());
    const float mixVal = m_apvts.getRawParameterValue("mix")->load() * 0.01f;

    updateScaleNotes(scaleIdx);

    // Update Granular params
    ff360::GranularParameters gp;
    gp.density = densityVal * (0.5f + evolveNorm * 0.5f);
    gp.grainSizeMs = ff360::lerp(120.0f, 40.0f, evolveNorm);
    gp.pitchSpraySemitones = evolveNorm * 7.0f;
    gp.evolveRate = evolveNorm;
    gp.mix = 1.0f;
    m_granularTexture.setParameters(gp);

    // Update Chorus params for Drone
    ff360::ModulationParameters mp;
    mp.rateHz = ff360::lerp(0.15f, 0.8f, evolveNorm);
    mp.depth = 0.6f;
    mp.width = 1.0f;
    mp.detune = 0.4f;
    mp.mix = 0.5f;
    mp.mode = ff360::ChorusMode::Quad4Voice;
    mp.character = ff360::ChorusCharacter::Vintage;
    m_droneChorus.setParameters(mp);

    // Update Reverb params
    ff360::ReverbParameters rp;
    rp.algorithm = ff360::ReverbAlgorithmType::Dream;
    rp.decayTime = ff360::lerp(2.5f, 15.0f, reverbWash);
    rp.preDelayMs = 25.0f;
    rp.width = 1.0f;
    rp.modulation = evolveNorm * 0.8f;
    rp.mix = reverbWash * 0.6f;
    m_reverbWash.setParameters(rp);

    float* left = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    const float sr = static_cast<float>(getSampleRate());
    const float invSr = 1.0f / sr;

    // Filter LFO frequency ~0.1 Hz
    const float lfoInc = ff360::TWO_PI * (0.05f + filterMove * 0.2f) * invSr;

    for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
        // 1. Synthesize Arp note from scale notes
        if (m_samplesUntilArpStep == 0) {
            m_samplesUntilArpStep = static_cast<size_t>(sr * 0.125f); // 1/16 note arp
            m_arpStep = (m_arpStep + 1) % std::max(size_t(1), m_currentScaleNotes.size());
        }
        m_samplesUntilArpStep--;

        const int noteSemi = m_currentScaleNotes[m_arpStep % m_currentScaleNotes.size()];
        const float arpFreq = 220.0f * std::pow(2.0f, (noteSemi + 12) / 12.0f);
        m_arpPhase += ff360::TWO_PI * arpFreq * invSr;
        if (m_arpPhase >= ff360::TWO_PI) m_arpPhase -= ff360::TWO_PI;

        const float arpSample = std::sin(m_arpPhase) * arpLvl * 0.35f;

        // 2. Synthesize base drone layer
        const float droneSample = std::sin(m_arpPhase * 0.5f) * droneLvl * 0.4f;

        // 3. Filter movement
        m_filterLfoPhase += lfoInc;
        if (m_filterLfoPhase >= ff360::TWO_PI) m_filterLfoPhase -= ff360::TWO_PI;
        const float cutoff = 600.0f + 2500.0f * (0.5f + 0.5f * std::sin(m_filterLfoPhase) * filterMove);

        m_filterL.configure(ff360::BiquadFilter::Type::Lowpass, sr, cutoff, 1.2f);
        m_filterR.configure(ff360::BiquadFilter::Type::Lowpass, sr, cutoff * 1.05f, 1.2f);

        float sigL = m_filterL.process(droneSample + arpSample);
        float sigR = m_filterR.process(droneSample + arpSample);

        left[i] = sigL;
        right[i] = sigR;
    }

    // 4. Drone chorus spread
    m_droneChorus.processStereo(left, right, static_cast<size_t>(numSamples));

    // 5. Granular texture layer
    std::vector<float> granL(numSamples, 0.0f);
    std::vector<float> granR(numSamples, 0.0f);
    m_granularTexture.processStereo(granL.data(), granR.data(), static_cast<size_t>(numSamples));

    for (int i = 0; i < numSamples; ++i) {
        left[i] += granL[i] * granLvl;
        right[i] += granR[i] * granLvl;
    }

    // 6. Reverb wash send
    m_reverbWash.processStereo(left, right, static_cast<size_t>(numSamples));

    // 7. Master Mix & Metering
    for (int i = 0; i < numSamples; ++i) {
        left[i] *= mixVal;
        right[i] *= mixVal;
    }

    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* NightDriveProcessor::createEditor() {
    return new NightDriveEditor(*this);
}

int NightDriveProcessor::getNumPrograms() { return static_cast<int>(m_presets.size()); }
int NightDriveProcessor::getCurrentProgram() { return m_currentPresetIndex; }
void NightDriveProcessor::setCurrentProgram(int index) {
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
const juce::String NightDriveProcessor::getProgramName(int index) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) return m_presets[index].name;
    return {};
}
void NightDriveProcessor::changeProgramName(int index, const juce::String& newName) {
    if (index >= 0 && index < static_cast<int>(m_presets.size())) m_presets[index].name = newName.toStdString();
}

void NightDriveProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NightDriveProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NightDriveProcessor();
}

#endif
