#include "ff360/GlitchEngine.h"
#include <cmath>
#include <algorithm>

namespace ff360 {

FF360_DSP_GlitchEngine::FF360_DSP_GlitchEngine() {
    reset();
}

void FF360_DSP_GlitchEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // 4 seconds capture ring buffer
    const size_t maxSamples = static_cast<size_t>(m_sampleRate * 4.0);
    m_captureBufferL.assign(maxSamples, 0.0f);
    m_captureBufferR.assign(maxSamples, 0.0f);

    setParameters(m_params);
    reset();
}

void FF360_DSP_GlitchEngine::reset() {
    std::fill(m_captureBufferL.begin(), m_captureBufferL.end(), 0.0f);
    std::fill(m_captureBufferR.begin(), m_captureBufferR.end(), 0.0f);
    m_writePos = 0;
    m_isGlitching = false;
    m_playbackPos = 0.0f;
    m_sliceStartPos = 0;
    m_samplesUntilNextGrid = 0;
    m_filterL.reset();
    m_filterR.reset();
    m_stepHistory.fill(false);
    m_stepHistoryPos = 0;
    m_scEnvelope = 0.0f;
    m_scWasAbove = false;
}

size_t FF360_DSP_GlitchEngine::calculateSliceSamples() const noexcept {
    const float bpm = std::max(20.0f, m_params.hostBpm);
    const float beatSec = 60.0f / bpm;
    float divFactor = 0.25f; // default 1/16th

    switch (m_params.division) {
        case GlitchTimingDivision::Div_1_4:  divFactor = 1.0f;   break;
        case GlitchTimingDivision::Div_1_8:  divFactor = 0.5f;   break;
        case GlitchTimingDivision::Div_1_16: divFactor = 0.25f;  break;
        case GlitchTimingDivision::Div_1_32: divFactor = 0.125f; break;
    }

    return std::max(static_cast<size_t>(32), static_cast<size_t>(beatSec * divFactor * m_sampleRate));
}

void FF360_DSP_GlitchEngine::setParameters(const GlitchParameters& params) noexcept {
    m_params = params;
    m_sliceLengthSamples = calculateSliceSamples();

    // Pitch playback rate factor: 2^(semitones / 12)
    m_playbackRate = std::pow(2.0f, m_params.pitchShiftSemitones / 12.0f);

    const float sr = static_cast<float>(m_sampleRate);
    m_filterL.configure(BiquadFilter::Type::Lowpass, sr, std::min(sr * 0.48f, m_params.filterCutoffHz), m_params.filterResonance);
    m_filterR.configure(BiquadFilter::Type::Lowpass, sr, std::min(sr * 0.48f, m_params.filterCutoffHz), m_params.filterResonance);
}

void FF360_DSP_GlitchEngine::triggerManualGlitch() {
    m_isGlitching = true;
    m_sliceStartPos = m_writePos;
    m_playbackPos = 0.0f;
}

void FF360_DSP_GlitchEngine::evaluateGridTrigger() {
    if (m_params.freeze) {
        m_isGlitching = true;
    } else if (m_rng.nextFloat() <= m_params.probability) {
        m_isGlitching = true;
        m_sliceStartPos = m_writePos;
        m_playbackPos = 0.0f;
    } else {
        m_isGlitching = false;
    }

    m_stepHistory[m_stepHistoryPos] = m_isGlitching;
    m_stepHistoryPos = (m_stepHistoryPos + 1) % kStepHistorySize;
}

void FF360_DSP_GlitchEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
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

void FF360_DSP_GlitchEngine::processStereo(float* left, float* right, size_t numSamples,
                                            const float* keyLeft, const float* keyRight) {
    if (m_captureBufferL.empty()) return;
    const size_t bufSize = m_captureBufferL.size();

    const bool scActive = (keyLeft != nullptr) && (m_params.sidechainSensitivity > 0.001f);
    // Higher sensitivity lowers the trigger threshold; at 0 it's effectively unreachable.
    const float scThreshold = lerp(1.0f, 0.04f, clamp(m_params.sidechainSensitivity, 0.0f, 1.0f));
    const float scRelease = 1.0f / (0.05f * static_cast<float>(m_sampleRate)); // ~50ms release

    // Bitcrush parameters (reusing tape engine quantization math)
    const float bits = lerp(16.0f, 4.0f, m_params.bitcrush);
    const float bitSteps = std::pow(2.0f, bits);
    const float invBitSteps = 1.0f / bitSteps;

    const float gateFraction = clamp(m_params.gate, 0.05f, 1.0f);
    const size_t activeGateSamples = static_cast<size_t>(m_sliceLengthSamples * gateFraction);

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];

        // 1. Continuously record into capture buffer (unless frozen)
        if (!m_params.freeze) {
            m_captureBufferL[m_writePos] = inL;
            m_captureBufferR[m_writePos] = inR;
            m_writePos = (m_writePos + 1) % bufSize;
        }

        // 2. Grid evaluation on rhythm subdivisions
        if (m_samplesUntilNextGrid == 0) {
            m_samplesUntilNextGrid = m_sliceLengthSamples;
            evaluateGridTrigger();
        }
        m_samplesUntilNextGrid--;

        // 2b. External sidechain transient trigger (e.g. "glitch on the kick"), independent
        // of the rhythm grid above. Also recorded into the step-grid history so the UI
        // visualizer reflects sidechain-triggered hits too.
        if (scActive) {
            const float keyAbs = std::max(std::abs(keyLeft[i]), std::abs(keyRight[i]));
            if (keyAbs > m_scEnvelope) {
                m_scEnvelope += (keyAbs - m_scEnvelope) * 0.6f; // fast attack
            } else {
                m_scEnvelope += (keyAbs - m_scEnvelope) * scRelease;
            }

            const bool aboveNow = m_scEnvelope > scThreshold;
            if (aboveNow && !m_scWasAbove) {
                m_isGlitching = true;
                m_sliceStartPos = m_writePos;
                m_playbackPos = 0.0f;
                m_stepHistory[m_stepHistoryPos] = true;
                m_stepHistoryPos = (m_stepHistoryPos + 1) % kStepHistorySize;
            }
            m_scWasAbove = aboveNow;
        }

        // 3. Playback / Stutter synthesis
        float glitchL = inL;
        float glitchR = inR;

        if (m_isGlitching) {
            // Check gate duty cycle
            const size_t currentPosInt = static_cast<size_t>(m_playbackPos);
            if (currentPosInt < activeGateSamples) {
                // Calculate read index with reverse option
                float readOffset = 0.0f;
                if (m_params.reverse) {
                    readOffset = static_cast<float>(m_sliceLengthSamples) - m_playbackPos;
                } else {
                    readOffset = m_playbackPos;
                }

                float readIdx = static_cast<float>(m_sliceStartPos) - readOffset;
                while (readIdx < 0.0f) readIdx += static_cast<float>(bufSize);

                const size_t idx0 = static_cast<size_t>(readIdx) % bufSize;
                const size_t idx1 = (idx0 + 1) % bufSize;
                const float frac = readIdx - static_cast<float>(idx0);

                // Linear interpolation on buffer slice
                glitchL = lerp(m_captureBufferL[idx0], m_captureBufferL[idx1], frac);
                glitchR = lerp(m_captureBufferR[idx0], m_captureBufferR[idx1], frac);
            } else {
                // Gated silence
                glitchL = 0.0f;
                glitchR = 0.0f;
            }

            // Advance playback position by pitch rate
            m_playbackPos += m_playbackRate;
            if (m_playbackPos >= static_cast<float>(m_sliceLengthSamples)) {
                m_playbackPos = 0.0f; // Loop stutter slice
            }
        }

        // 4. Bitcrush reduction
        if (m_params.bitcrush > 0.001f) {
            glitchL = std::round(glitchL * bitSteps) * invBitSteps;
            glitchR = std::round(glitchR * bitSteps) * invBitSteps;
        }

        // 5. Resonant filter
        glitchL = m_filterL.process(glitchL);
        glitchR = m_filterR.process(glitchR);

        // 6. Dry/Wet Mix
        left[i] = lerp(inL, glitchL, m_params.mix);
        right[i] = lerp(inR, glitchR, m_params.mix);
    }
}

} // namespace ff360
