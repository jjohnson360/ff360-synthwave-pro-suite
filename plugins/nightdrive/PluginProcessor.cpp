#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NightDriveProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Display names follow the UI mockup's night-drive ambience theme; parameter IDs are
    // left untouched so existing sessions/automation keep working.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "dronelevel", 1 }, "Road Hum", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "granularlevel", 1 }, "Rain Texture", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "arplevel", 1 }, "Neon Arp", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "evolve", 1 }, "Evolve Macro", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "density", 1 }, "City Density", juce::NormalisableRange<float>(5.0f, 60.0f, 0.1f), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel("gr/s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "filtermove", 1 }, "Engine Movement", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "reverbwash", 1 }, "Atmos Wash", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 65.0f,
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "scduck", 1 }, "Sidechain Duck", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Output trim, bypass (ff360::FF360_DSP_OutputStage)
    ff360_ui::output::addParameters(params, false);

    return { params.begin(), params.end() };
}

NightDriveProcessor::NightDriveProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                     .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      m_apvts(*this, nullptr, "Parameters", createParameterLayout()),
      m_history(*this, {}, { "bypass" }),
      m_presetManager(m_apvts, m_history, "NightDrive", ff360::getNightDrivePresets(), { "bypass" }) {
    updateScaleNotes(1); // Natural Minor default
}

bool NightDriveProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    const auto scSet = layouts.getChannelSet(true, 1);
    return scSet.isDisabled() || scSet == juce::AudioChannelSet::mono() || scSet == juce::AudioChannelSet::stereo();
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
    m_sampleRate = sampleRate;
    m_granL.assign(static_cast<size_t>(samplesPerBlock), 0.0f);
    m_granR.assign(static_cast<size_t>(samplesPerBlock), 0.0f);
    m_outputStage.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_droneChorus.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_granularTexture.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_reverbWash.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_meteringBridge.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    m_paramManager.prepare(sampleRate);

    m_filterL.configure(ff360::BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 3000.0f, 1.5f);
    m_filterR.configure(ff360::BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 3000.0f, 1.5f);

    m_arpPhase = 0.0f;
    m_arpStep = 0;
    m_samplesUntilArpStep = static_cast<size_t>(sampleRate * 0.125); // 1/16th note at 120bpm; retuned once host tempo is known
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

    // Keep the input for bypass and auto gain
    m_outputStage.captureDry(buffer.getReadPointer(0), buffer.getReadPointer(numChannels > 1 ? 1 : 0), static_cast<size_t>(numSamples));

    // Detect ChordFlow MIDI / Chord messages
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            m_chordFlowDetected = true;
        }
    }

    // Read host tempo so the arp locks to the song, not a fixed internal clock
    float hostBpm = 120.0f;
    if (auto* playHead = getPlayHead()) {
        if (auto pos = playHead->getPosition()) {
            if (pos->getBpm().hasValue()) hostBpm = static_cast<float>(*pos->getBpm());
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
    const float scDuckAmt = m_apvts.getRawParameterValue("scduck")->load() * 0.01f;

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

    const float sr = static_cast<float>(m_sampleRate);
    const float invSr = 1.0f / sr;

    // Filter LFO frequency ~0.1 Hz
    const float lfoInc = ff360::TWO_PI * (0.05f + filterMove * 0.2f) * invSr;

    // 1/16 note length at the host's current tempo (recomputed every block, applied at the next step boundary)
    const float sixteenthNoteSec = (60.0f / std::max(20.0f, hostBpm)) / 4.0f;
    const size_t sixteenthNoteSamples = std::max(static_cast<size_t>(1), static_cast<size_t>(sr * sixteenthNoteSec));

    for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
        // 1. Synthesize Arp note from scale notes
        if (m_samplesUntilArpStep == 0) {
            m_samplesUntilArpStep = sixteenthNoteSamples; // tempo-synced 1/16 note arp
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
    if (static_cast<size_t>(numSamples) > m_granL.size()) { // bigger block than prepared (rare)
        m_granL.resize(static_cast<size_t>(numSamples));
        m_granR.resize(static_cast<size_t>(numSamples));
    }
    std::fill_n(m_granL.begin(), numSamples, 0.0f);
    std::fill_n(m_granR.begin(), numSamples, 0.0f);
    m_granularTexture.processStereo(m_granL.data(), m_granR.data(), static_cast<size_t>(numSamples));

    for (int i = 0; i < numSamples; ++i) {
        left[i] += m_granL[static_cast<size_t>(i)] * granLvl;
        right[i] += m_granR[static_cast<size_t>(i)] * granLvl;
    }

    // 6. Reverb wash send
    m_reverbWash.processStereo(left, right, static_cast<size_t>(numSamples));

    // 7. Sidechain ducking: pull the ambient bed down under an external key (e.g. lead/vocal).
    // Only meaningful when a sidechain input is actually connected — NightDrive generates its
    // own signal, so unlike Midnight Reverb there's no "self" input to fall back to.
    auto scBuffer = getBusBuffer(buffer, true, 1);
    const bool scConnected = scBuffer.getNumChannels() > 0 && scDuckAmt > 0.0f;
    const float* scLeft = scConnected ? scBuffer.getReadPointer(0) : nullptr;
    const float* scRight = scConnected ? (scBuffer.getNumChannels() > 1 ? scBuffer.getReadPointer(1) : scLeft) : nullptr;

    // 8. Master Mix & Metering
    for (int i = 0; i < numSamples; ++i) {
        float duckGain = 1.0f;
        if (scConnected) {
            const float keyAbs = std::max(std::abs(scLeft[i]), std::abs(scRight[i]));
            if (keyAbs > m_scDuckEnvelope) {
                m_scDuckEnvelope += (keyAbs - m_scDuckEnvelope) * 0.5f; // fast attack
            } else {
                m_scDuckEnvelope += (keyAbs - m_scDuckEnvelope) * (invSr * 12.0f); // ~80-150ms release
            }
            duckGain = ff360::clamp(1.0f - (m_scDuckEnvelope * scDuckAmt * 1.5f), 0.0f, 1.0f);
        }

        left[i] *= mixVal * duckGain;
        right[i] *= mixVal * duckGain;
    }

    m_outputStage.process(left, right, static_cast<size_t>(numSamples), ff360_ui::output::readSettings(m_apvts));
    m_meteringBridge.processStereo(left, right, static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* NightDriveProcessor::createEditor() {
    return new NightDriveEditor(*this);
}

// Presets live in the plugin's own menu (ff360_ui::WorkflowBar); the host sees a single program
int NightDriveProcessor::getNumPrograms() { return 1; }
int NightDriveProcessor::getCurrentProgram() { return 0; }
void NightDriveProcessor::setCurrentProgram(int) {}
const juce::String NightDriveProcessor::getProgramName(int) { return m_presetManager.getCurrentName(); }
void NightDriveProcessor::changeProgramName(int, const juce::String&) {}

void NightDriveProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    m_history.writeState(*xml);
    m_presetManager.writeState(*xml);
    xml->setAttribute("uiScale", (double)m_editorScale.load());
    copyXmlToBinary(*xml, destData);
}

void NightDriveProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts.state.getType())) {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        m_history.readState(*xmlState);
        m_presetManager.readState(*xmlState);
        m_editorScale.store((float)xmlState->getDoubleAttribute("uiScale", 1.0));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NightDriveProcessor();
}

#endif
