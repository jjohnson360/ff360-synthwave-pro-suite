#include "ff360/ModulationEngine.h"
#include <cmath>

namespace ff360 {

FF360_DSP_ModulationEngine::FF360_DSP_ModulationEngine() {
    setupVoicePans();
    reset();
}

void FF360_DSP_ModulationEngine::setupVoicePans() {
    // 4 Voices with distinct quadrature & golden-ratio phase/pan distribution
    // Voice 0: Hard Left / Mid Left
    m_voices[0].phaseOffset = 0.0f;
    m_voices[0].panL = 1.0f;
    m_voices[0].panR = 0.0f;

    // Voice 1: Hard Right / Mid Right
    m_voices[1].phaseOffset = PI * 0.5f; // 90 deg quadrature
    m_voices[1].panL = 0.0f;
    m_voices[1].panR = 1.0f;

    // Voice 2: Mid-Left
    m_voices[2].phaseOffset = PI;        // 180 deg
    m_voices[2].panL = 0.8f;
    m_voices[2].panR = 0.2f;

    // Voice 3: Mid-Right
    m_voices[3].phaseOffset = PI * 1.5f; // 270 deg
    m_voices[3].panL = 0.2f;
    m_voices[3].panR = 0.8f;
}

void FF360_DSP_ModulationEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // 50ms delay line buffer per voice
    const size_t maxDelaySamples = static_cast<size_t>(m_sampleRate * 0.08) + 128;
    for (auto& voice : m_voices) {
        voice.delayLine.init(maxDelaySamples);
    }

    const float sr = static_cast<float>(m_sampleRate);
    const float cutoff = std::max(20.0f, m_params.bassMonoCrossoverHz);
    m_bassLpL1.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpL2.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpR1.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpR2.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);

    m_bassHpL1.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpL2.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpR1.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpR2.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);

    m_vintageLpL.setType(OnePoleFilter::Type::Lowpass);
    m_vintageLpR.setType(OnePoleFilter::Type::Lowpass);
    m_vintageLpL.setCutoff(sr, 14000.0f);
    m_vintageLpR.setCutoff(sr, 14000.0f);

    reset();
}

void FF360_DSP_ModulationEngine::reset() {
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        m_voices[i].delayLine.reset();
        m_voices[i].lfoPhase = m_voices[i].phaseOffset;
        m_voices[i].feedbackSample = 0.0f;
        m_voices[i].driftPhase = static_cast<float>(i) * 1.618f;
    }
    m_bassLpL1.reset(); m_bassLpL2.reset();
    m_bassLpR1.reset(); m_bassLpR2.reset();
    m_bassHpL1.reset(); m_bassHpL2.reset();
    m_bassHpR1.reset(); m_bassHpR2.reset();
    m_vintageLpL.reset();
    m_vintageLpR.reset();
}

void FF360_DSP_ModulationEngine::setParameters(const ModulationParameters& params) noexcept {
    m_params = params;
    const float sr = static_cast<float>(m_sampleRate);
    const float cutoff = std::max(20.0f, m_params.bassMonoCrossoverHz);
    m_bassLpL1.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpL2.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpR1.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);
    m_bassLpR2.configure(BiquadFilter::Type::Lowpass, sr, cutoff, 0.7071f);

    m_bassHpL1.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpL2.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpR1.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
    m_bassHpR2.configure(BiquadFilter::Type::Highpass, sr, cutoff, 0.7071f);
}

void FF360_DSP_ModulationEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
    if (numChannels == 0 || numSamples == 0) return;

    if (numChannels == 1) {
        std::vector<float> tempL(inputs[0], inputs[0] + numSamples);
        std::vector<float> tempR(inputs[0], inputs[0] + numSamples);
        processStereo(tempL.data(), tempR.data(), numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            outputs[0][i] = 0.5f * (tempL[i] + tempR[i]);
        }
    } else {
        if (inputs[0] != outputs[0]) std::copy(inputs[0], inputs[0] + numSamples, outputs[0]);
        if (inputs[1] != outputs[1]) std::copy(inputs[1], inputs[1] + numSamples, outputs[1]);
        processStereo(outputs[0], outputs[1], numSamples);
    }
}

void FF360_DSP_ModulationEngine::processStereo(float* left, float* right, size_t numSamples) {
    const float sr = static_cast<float>(m_sampleRate);
    const float invSr = 1.0f / sr;
    const size_t activeVoiceCount = (m_params.mode == ChorusMode::Quad4Voice) ? 4 : 2;
    const float voiceGain = 1.0f / std::sqrt(static_cast<float>(activeVoiceCount));

    const float basePreDelaySamples = (m_params.preDelayMs * 0.001f) * sr + 40.0f; // min ~1ms offset
    const float maxExcursionSamples = m_params.depth * (0.006f * sr); // up to 6ms depth
    const float lfoRateBase = m_params.rateHz;
    const float feedback = clamp(m_params.feedback, 0.0f, 0.95f);
    const float width = m_params.width;
    const bool isVintage = (m_params.character == ChorusCharacter::Vintage);

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];

        float wetVoiceAccumL = 0.0f;
        float wetVoiceAccumR = 0.0f;

        for (size_t v = 0; v < activeVoiceCount; ++v) {
            auto& voice = m_voices[v];

            // Voice input with cross feedback
            const float voiceIn = (v % 2 == 0 ? inL : inR) + voice.feedbackSample * feedback;
            voice.delayLine.write(voiceIn);

            // Detune rate spread per voice
            const float detuneFactor = 1.0f + (static_cast<float>(v) - 0.5f * static_cast<float>(activeVoiceCount - 1)) * (m_params.detune * 0.18f);
            const float voiceRate = lfoRateBase * detuneFactor;

            // Increment LFO phase
            voice.lfoPhase += TWO_PI * voiceRate * invSr;
            if (voice.lfoPhase >= TWO_PI) voice.lfoPhase -= TWO_PI;

            // Instability primitive for Vintage character
            float instability = 0.0f;
            if (isVintage) {
                voice.driftPhase += TWO_PI * (0.4f + static_cast<float>(v) * 0.2f) * invSr;
                if (voice.driftPhase >= TWO_PI) voice.driftPhase -= TWO_PI;
                instability = std::sin(voice.driftPhase) * (0.0006f * sr); // ~0.6ms analog drift
            }

            // Triangle / sine hybrid modulation shape for smooth 80s BBD chorus feel
            const float sinVal = std::sin(voice.lfoPhase);
            const float delayMod = (sinVal * 0.5f + 0.5f) * maxExcursionSamples + instability;
            const float totalDelay = basePreDelaySamples + delayMod;

            const float delayedSample = voice.delayLine.readFractional(totalDelay);
            voice.feedbackSample = std::tanh(delayedSample); // Soft limit feedback

            // Stereo panning calculation
            float panL = voice.panL;
            float panR = voice.panR;
            panL = lerp(0.5f, panL, width);
            panR = lerp(0.5f, panR, width);

            wetVoiceAccumL += delayedSample * panL * voiceGain;
            wetVoiceAccumR += delayedSample * panR * voiceGain;
        }

        // Vintage tone coloring (subtle high-end rolloff)
        if (isVintage) {
            wetVoiceAccumL = m_vintageLpL.process(wetVoiceAccumL);
            wetVoiceAccumR = m_vintageLpR.process(wetVoiceAccumR);
        }

        // Bass Mono Crossover Processing (4th-order Linkwitz-Riley)
        if (m_params.bassMonoCrossoverHz > 10.0f) {
            const float lowL = m_bassLpL2.process(m_bassLpL1.process(wetVoiceAccumL));
            const float lowR = m_bassLpR2.process(m_bassLpR1.process(wetVoiceAccumR));
            const float monoLow = 0.5f * (lowL + lowR);

            const float highL = m_bassHpL2.process(m_bassHpL1.process(wetVoiceAccumL));
            const float highR = m_bassHpR2.process(m_bassHpR1.process(wetVoiceAccumR));

            wetVoiceAccumL = monoLow + highL;
            wetVoiceAccumR = monoLow + highR;
        }

        // Dry/Wet Mix
        left[i] = lerp(inL, wetVoiceAccumL, m_params.mix);
        right[i] = lerp(inR, wetVoiceAccumR, m_params.mix);
    }
}

} // namespace ff360
