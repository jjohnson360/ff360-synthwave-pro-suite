#include "ff360/TapeEngine.h"
#include <cmath>

namespace ff360 {

FF360_DSP_TapeEngine::FF360_DSP_TapeEngine() {
    reset();
}

void FF360_DSP_TapeEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // Allocate delay line buffer (~100ms max delay for wow/flutter/drift)
    const size_t maxDelaySamples = static_cast<size_t>(m_sampleRate * 0.1) + 64;
    m_delayLineL.init(maxDelaySamples);
    m_delayLineR.init(maxDelaySamples);

    m_pinkFilterL.setType(OnePoleFilter::Type::Lowpass);
    m_pinkFilterR.setType(OnePoleFilter::Type::Lowpass);
    m_pinkFilterL.setCutoff(static_cast<float>(m_sampleRate), 350.0f);
    m_pinkFilterR.setCutoff(static_cast<float>(m_sampleRate), 350.0f);

    updateFilters();
    reset();
}

void FF360_DSP_TapeEngine::reset() {
    m_delayLineL.reset();
    m_delayLineR.reset();
    m_hfFilterL.reset();
    m_hfFilterR.reset();
    m_pinkFilterL.reset();
    m_pinkFilterR.reset();
    m_hissFilterL.reset();
    m_hissFilterR.reset();

    m_wowPhase = 0.0f;
    m_flutterPhase1 = 0.0f;
    m_flutterPhase2 = 0.0f;
    m_driftPhase = 0.0f;
    m_warblePhaseL = 0.0f;
    m_warblePhaseR = 0.0f;

    m_dropoutGainL = 1.0f;
    m_dropoutGainR = 1.0f;
    m_dropoutTimerL = 0.0f;
    m_dropoutTimerR = 0.0f;

    m_loFiPhase = 0.0f;
    m_loFiHoldSampleL = 0.0f;
    m_loFiHoldSampleR = 0.0f;

    m_hysteresisStateL = 0.0f;
    m_hysteresisStateR = 0.0f;
}

void FF360_DSP_TapeEngine::setParameters(const TapeParameters& params) noexcept {
    m_params = params;
    updateFilters();
}

void FF360_DSP_TapeEngine::setParameter(TapeParamId id, float normalizedValue) noexcept {
    const float val = clamp(normalizedValue, 0.0f, 1.0f);
    switch (id) {
        case TapeParamId::Saturation:         m_params.saturation = val; break;
        case TapeParamId::Wow:                m_params.wow = val; break;
        case TapeParamId::Flutter:            m_params.flutter = val; break;
        case TapeParamId::PitchDrift:         m_params.pitchDrift = val; break;
        case TapeParamId::Noise:              m_params.noise = val; break;
        case TapeParamId::Hiss:               m_params.hiss = val; break;
        case TapeParamId::Dropouts:           m_params.dropouts = val; break;
        case TapeParamId::LoFi:               m_params.loFi = val; break;
        case TapeParamId::BitReduction:       m_params.bitReduction = val; break;
        case TapeParamId::HighFrequencyLoss:  m_params.highFrequencyLoss = val; break;
        case TapeParamId::StereoDrift:        m_params.stereoDrift = val; break;
        case TapeParamId::Warble:             m_params.warble = val; break;
        case TapeParamId::InputGain:          m_params.inputGainDb = (val * 48.0f) - 24.0f; break;
        case TapeParamId::OutputGain:         m_params.outputGainDb = (val * 48.0f) - 24.0f; break;
        case TapeParamId::Mix:                m_params.mix = val; break;
        default: break;
    }
    updateFilters();
}

float FF360_DSP_TapeEngine::getParameter(TapeParamId id) const noexcept {
    switch (id) {
        case TapeParamId::Saturation:         return m_params.saturation;
        case TapeParamId::Wow:                return m_params.wow;
        case TapeParamId::Flutter:            return m_params.flutter;
        case TapeParamId::PitchDrift:         return m_params.pitchDrift;
        case TapeParamId::Noise:              return m_params.noise;
        case TapeParamId::Hiss:               return m_params.hiss;
        case TapeParamId::Dropouts:           return m_params.dropouts;
        case TapeParamId::LoFi:               return m_params.loFi;
        case TapeParamId::BitReduction:       return m_params.bitReduction;
        case TapeParamId::HighFrequencyLoss:  return m_params.highFrequencyLoss;
        case TapeParamId::StereoDrift:        return m_params.stereoDrift;
        case TapeParamId::Warble:             return m_params.warble;
        case TapeParamId::InputGain:          return (m_params.inputGainDb + 24.0f) / 48.0f;
        case TapeParamId::OutputGain:         return (m_params.outputGainDb + 24.0f) / 48.0f;
        case TapeParamId::Mix:                return m_params.mix;
        default: return 0.0f;
    }
}

void FF360_DSP_TapeEngine::updateFilters() {
    const float sr = static_cast<float>(m_sampleRate);
    
    // HF loss: 20kHz down to 2kHz
    const float hfCutoff = lerp(20000.0f, 2200.0f, m_params.highFrequencyLoss);
    m_hfFilterL.configure(BiquadFilter::Type::Lowpass, sr, hfCutoff, 0.707f);
    m_hfFilterR.configure(BiquadFilter::Type::Lowpass, sr, hfCutoff, 0.707f);

    // Hiss bandpass: centred around 7kHz
    m_hissFilterL.configure(BiquadFilter::Type::HighShelf, sr, 6500.0f, 0.707f, 3.0f);
    m_hissFilterR.configure(BiquadFilter::Type::HighShelf, sr, 6500.0f, 0.707f, 3.0f);
}

void FF360_DSP_TapeEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
    if (numChannels == 0 || numSamples == 0) return;

    if (numChannels == 1) {
        std::vector<float> tempL(inputs[0], inputs[0] + numSamples);
        std::vector<float> tempR(inputs[0], inputs[0] + numSamples);
        processStereo(tempL.data(), tempR.data(), numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            outputs[0][i] = 0.5f * (tempL[i] + tempR[i]);
        }
    } else {
        // Copy to outputs if not in-place
        if (inputs[0] != outputs[0]) std::copy(inputs[0], inputs[0] + numSamples, outputs[0]);
        if (inputs[1] != outputs[1]) std::copy(inputs[1], inputs[1] + numSamples, outputs[1]);
        processStereo(outputs[0], outputs[1], numSamples);
    }
}

void FF360_DSP_TapeEngine::processStereo(float* left, float* right, size_t numSamples) {
    const float sr = static_cast<float>(m_sampleRate);
    const float invSr = 1.0f / sr;
    const float inGain = dbToGain(m_params.inputGainDb);
    const float outGain = dbToGain(m_params.outputGainDb);
    const float mix = m_params.mix;

    // Nominal center delay for wow/flutter modulation buffer
    const float centerDelay = 512.0f;

    // Increments for oscillators
    const float wowInc = TWO_PI * 0.85f * invSr;            // ~0.85 Hz wow
    const float flutter1Inc = TWO_PI * 8.2f * invSr;         // 8.2 Hz flutter
    const float flutter2Inc = TWO_PI * 13.7f * invSr;        // 13.7 Hz flutter secondary
    const float driftInc = TWO_PI * 0.12f * invSr;           // 0.12 Hz slow drift
    const float warbleIncL = TWO_PI * 3.4f * invSr;
    const float warbleIncR = TWO_PI * 3.7f * invSr;

    // Bit reduction levels: 16 down to 4 bits
    const float bits = lerp(16.0f, 4.0f, m_params.bitReduction);
    const float bitSteps = std::pow(2.0f, bits);
    const float invBitSteps = 1.0f / bitSteps;

    // LoFi sample hold step factor: 1 (no decimation) to ~12 (heavy decimation)
    const float loFiStep = 1.0f + m_params.loFi * 11.0f;

    // Noise/Hiss amplitudes
    const float noiseAmp = m_params.noise * 0.035f;
    const float hissAmp = m_params.hiss * 0.025f;

    for (size_t i = 0; i < numSamples; ++i) {
        const float dryL = left[i];
        const float dryR = right[i];

        // 1. Input Gain
        float sigL = dryL * inGain;
        float sigR = dryR * inGain;

        // 2. Tape Saturation with dynamic hysteresis
        if (m_params.saturation > 0.001f) {
            const float drive = 1.0f + m_params.saturation * 6.0f;
            const float hystAmount = m_params.saturation * 0.15f;

            // Channel L saturation
            float drivenL = sigL * drive + m_hysteresisStateL * hystAmount;
            // Soft-knee asymmetric tape curve
            float satL = std::tanh(drivenL) * (1.0f - 0.05f * std::tanh(drivenL * 1.5f));
            m_hysteresisStateL = satL - drivenL * 0.3f;
            sigL = lerp(sigL, satL / std::sqrt(drive), m_params.saturation);

            // Channel R saturation
            float drivenR = sigR * drive + m_hysteresisStateR * hystAmount;
            float satR = std::tanh(drivenR) * (1.0f - 0.05f * std::tanh(drivenR * 1.5f));
            m_hysteresisStateR = satR - drivenR * 0.3f;
            sigR = lerp(sigR, satR / std::sqrt(drive), m_params.saturation);
        }

        // 3. Write to Delay Line for Pitch Modulations
        m_delayLineL.write(sigL);
        m_delayLineR.write(sigR);

        // Update oscillator phases
        m_wowPhase += wowInc;
        if (m_wowPhase >= TWO_PI) m_wowPhase -= TWO_PI;

        m_flutterPhase1 += flutter1Inc;
        if (m_flutterPhase1 >= TWO_PI) m_flutterPhase1 -= TWO_PI;

        m_flutterPhase2 += flutter2Inc;
        if (m_flutterPhase2 >= TWO_PI) m_flutterPhase2 -= TWO_PI;

        m_driftPhase += driftInc;
        if (m_driftPhase >= TWO_PI) m_driftPhase -= TWO_PI;

        m_warblePhaseL += warbleIncL;
        if (m_warblePhaseL >= TWO_PI) m_warblePhaseL -= TWO_PI;

        m_warblePhaseR += warbleIncR;
        if (m_warblePhaseR >= TWO_PI) m_warblePhaseR -= TWO_PI;

        // Calculate modulation amounts in samples
        const float wowMod = std::sin(m_wowPhase) * (m_params.wow * 45.0f);
        const float flutterMod = (0.6f * std::sin(m_flutterPhase1) + 0.4f * std::sin(m_flutterPhase2)) * (m_params.flutter * 12.0f);
        const float driftMod = std::sin(m_driftPhase) * (m_params.pitchDrift * 30.0f);
        const float warbleModL = std::sin(m_warblePhaseL + std::sin(m_wowPhase) * 1.2f) * (m_params.warble * 22.0f);
        const float warbleModR = std::sin(m_warblePhaseR + std::cos(m_wowPhase) * 1.2f) * (m_params.warble * 22.0f);

        // Stereo Drift offset
        const float stereoDelayOffset = m_params.stereoDrift * 28.0f;

        const float totalDelayL = centerDelay + wowMod + flutterMod + driftMod + warbleModL - stereoDelayOffset * 0.5f;
        const float totalDelayR = centerDelay + (wowMod * 0.95f) + (flutterMod * 1.05f) + (driftMod * 0.9f) + warbleModR + stereoDelayOffset * 0.5f;

        sigL = m_delayLineL.readFractional(totalDelayL);
        sigR = m_delayLineR.readFractional(totalDelayR);

        // 4. High-Frequency Loss
        if (m_params.highFrequencyLoss > 0.001f) {
            sigL = m_hfFilterL.process(sigL);
            sigR = m_hfFilterR.process(sigR);
        }

        // 5. Lo-Fi & Bit Reduction
        if (m_params.loFi > 0.001f) {
            m_loFiPhase += 1.0f;
            if (m_loFiPhase >= loFiStep) {
                m_loFiPhase -= loFiStep;
                m_loFiHoldSampleL = sigL;
                m_loFiHoldSampleR = sigR;
            }
            sigL = m_loFiHoldSampleL;
            sigR = m_loFiHoldSampleR;
        }

        if (m_params.bitReduction > 0.001f) {
            const float quantL = std::round(sigL * bitSteps) * invBitSteps;
            const float quantR = std::round(sigR * bitSteps) * invBitSteps;
            sigL = lerp(sigL, quantL, m_params.bitReduction);
            sigR = lerp(sigR, quantR, m_params.bitReduction);
        }

        // 6. Dropouts
        if (m_params.dropouts > 0.001f) {
            m_dropoutTimerL -= invSr;
            if (m_dropoutTimerL <= 0.0f) {
                if (m_rng.nextFloat() < (m_params.dropouts * 0.04f)) {
                    m_dropoutGainL = lerp(0.05f, 0.4f, m_rng.nextFloat());
                    m_dropoutTimerL = lerp(0.02f, 0.18f, m_rng.nextFloat());
                } else {
                    m_dropoutTimerL = lerp(0.05f, 0.3f, m_rng.nextFloat());
                }
            }
            m_dropoutGainL += (1.0f - m_dropoutGainL) * (18.0f * invSr);
            sigL *= m_dropoutGainL;

            m_dropoutTimerR -= invSr;
            if (m_dropoutTimerR <= 0.0f) {
                if (m_rng.nextFloat() < (m_params.dropouts * 0.04f)) {
                    m_dropoutGainR = lerp(0.05f, 0.4f, m_rng.nextFloat());
                    m_dropoutTimerR = lerp(0.02f, 0.18f, m_rng.nextFloat());
                } else {
                    m_dropoutTimerR = lerp(0.05f, 0.3f, m_rng.nextFloat());
                }
            }
            m_dropoutGainR += (1.0f - m_dropoutGainR) * (18.0f * invSr);
            sigR *= m_dropoutGainR;
        }

        // 7. Noise and Hiss Injection
        if (m_params.noise > 0.001f) {
            const float rawNoiseL = m_rng.nextSignedFloat();
            const float rawNoiseR = m_rng.nextSignedFloat();
            const float pinkL = m_pinkFilterL.process(rawNoiseL);
            const float pinkR = m_pinkFilterR.process(rawNoiseR);
            sigL += pinkL * noiseAmp;
            sigR += pinkR * noiseAmp;
        }

        if (m_params.hiss > 0.001f) {
            const float rawHissL = m_rng.nextSignedFloat();
            const float rawHissR = m_rng.nextSignedFloat();
            const float hissL = m_hissFilterL.process(rawHissL);
            const float hissR = m_hissFilterR.process(rawHissR);
            sigL += hissL * hissAmp;
            sigR += hissR * hissAmp;
        }

        // 8. Output Gain and Dry/Wet Mix
        const float wetL = sigL * outGain;
        const float wetR = sigR * outGain;

        left[i] = lerp(dryL, wetL, mix);
        right[i] = lerp(dryR, wetR, mix);
    }
}

} // namespace ff360
