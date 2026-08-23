#include "ff360/StereoEngine.h"
#include <cmath>
#include <algorithm>

namespace ff360 {

FF360_DSP_StereoEngine::FF360_DSP_StereoEngine() {
    reset();
}

void FF360_DSP_StereoEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // 50ms buffer for Haas / micro-delay
    const size_t maxDelaySamples = static_cast<size_t>(m_sampleRate * 0.06) + 128;
    m_delayLineL.init(maxDelaySamples);
    m_delayLineR.init(maxDelaySamples);

    // Correlation integration window (~50ms)
    m_corrWindowSize = static_cast<size_t>(m_sampleRate * 0.05);
    if (m_corrWindowSize < 64) m_corrWindowSize = 64;
    m_corrBufferL.assign(m_corrWindowSize, 0.0f);
    m_corrBufferR.assign(m_corrWindowSize, 0.0f);

    updateFilters();
    reset();
}

void FF360_DSP_StereoEngine::reset() {
    m_delayLineL.reset();
    m_delayLineR.reset();
    m_detunePhaseL = 0.0f;
    m_detunePhaseR = 0.0f;

    m_bassLpL1.reset(); m_bassLpL2.reset();
    m_bassLpR1.reset(); m_bassLpR2.reset();
    m_bassHpL1.reset(); m_bassHpL2.reset();
    m_bassHpR1.reset(); m_bassHpR2.reset();

    m_freqLpL1.reset(); m_freqLpL2.reset();
    m_freqLpR1.reset(); m_freqLpR2.reset();
    m_freqHpL1.reset(); m_freqHpL2.reset();
    m_freqHpR1.reset(); m_freqHpR2.reset();

    std::fill(m_corrBufferL.begin(), m_corrBufferL.end(), 0.0f);
    std::fill(m_corrBufferR.begin(), m_corrBufferR.end(), 0.0f);
    m_corrWritePos = 0;
    m_sumL2 = 0.0;
    m_sumR2 = 0.0;
    m_sumLR = 0.0;

    m_metrics = StereoMetrics{};
}

void FF360_DSP_StereoEngine::setParameters(const StereoParameters& params) noexcept {
    m_params = params;
    updateFilters();
}

void FF360_DSP_StereoEngine::updateFilters() {
    const float sr = static_cast<float>(m_sampleRate);

    // Bass mono crossover
    const float bassCutoff = std::max(20.0f, m_params.bassMonoCutoffHz);
    m_bassLpL1.configure(BiquadFilter::Type::Lowpass, sr, bassCutoff, 0.7071f);
    m_bassLpL2.configure(BiquadFilter::Type::Lowpass, sr, bassCutoff, 0.7071f);
    m_bassLpR1.configure(BiquadFilter::Type::Lowpass, sr, bassCutoff, 0.7071f);
    m_bassLpR2.configure(BiquadFilter::Type::Lowpass, sr, bassCutoff, 0.7071f);

    m_bassHpL1.configure(BiquadFilter::Type::Highpass, sr, bassCutoff, 0.7071f);
    m_bassHpL2.configure(BiquadFilter::Type::Highpass, sr, bassCutoff, 0.7071f);
    m_bassHpR1.configure(BiquadFilter::Type::Highpass, sr, bassCutoff, 0.7071f);
    m_bassHpR2.configure(BiquadFilter::Type::Highpass, sr, bassCutoff, 0.7071f);

    // Frequency-dependent width crossover
    const float freqCutoff = clamp(m_params.freqWidthCrossoverHz, 100.0f, sr * 0.45f);
    m_freqLpL1.configure(BiquadFilter::Type::Lowpass, sr, freqCutoff, 0.7071f);
    m_freqLpL2.configure(BiquadFilter::Type::Lowpass, sr, freqCutoff, 0.7071f);
    m_freqLpR1.configure(BiquadFilter::Type::Lowpass, sr, freqCutoff, 0.7071f);
    m_freqLpR2.configure(BiquadFilter::Type::Lowpass, sr, freqCutoff, 0.7071f);

    m_freqHpL1.configure(BiquadFilter::Type::Highpass, sr, freqCutoff, 0.7071f);
    m_freqHpL2.configure(BiquadFilter::Type::Highpass, sr, freqCutoff, 0.7071f);
    m_freqHpR1.configure(BiquadFilter::Type::Highpass, sr, freqCutoff, 0.7071f);
    m_freqHpR2.configure(BiquadFilter::Type::Highpass, sr, freqCutoff, 0.7071f);
}

void FF360_DSP_StereoEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
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

void FF360_DSP_StereoEngine::processStereo(float* left, float* right, size_t numSamples) {
    const float sr = static_cast<float>(m_sampleRate);
    const float invSr = 1.0f / sr;

    // 1. Micro-delay & Haas delay offsets in samples
    const float microDelaySamples = (m_params.microDelayMs * 0.001f) * sr;
    const float haasDelaySamples = (m_params.haasWidth * 0.018f) * sr; // up to 18ms Haas offset

    // 2. Stereo Rotation matrix angle (convert degrees to radians)
    const float rotRad = (m_params.stereoRotationDeg * PI) / 180.0f;
    const float cosRot = std::cos(rotRad);
    const float sinRot = std::sin(rotRad);

    // 3. Detune LFO rate (~0.2 Hz)
    const float detuneInc = TWO_PI * 0.25f * invSr;

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];

        // Write to delay lines
        m_delayLineL.write(inL);
        m_delayLineR.write(inR);

        // Calculate detune pitch modulation
        m_detunePhaseL += detuneInc;
        m_detunePhaseR += detuneInc * 1.25f;
        if (m_detunePhaseL >= TWO_PI) m_detunePhaseL -= TWO_PI;
        if (m_detunePhaseR >= TWO_PI) m_detunePhaseR -= TWO_PI;

        const float detuneModL = std::sin(m_detunePhaseL) * (m_params.stereoDetune * 0.002f * sr);
        const float detuneModR = std::sin(m_detunePhaseR) * (m_params.stereoDetune * 0.002f * sr);

        // Read with Haas / Micro-delay / Detune
        const float delL = microDelaySamples + detuneModL;
        const float delR = haasDelaySamples + detuneModR;

        float sigL = m_delayLineL.readFractional(delL);
        float sigR = m_delayLineR.readFractional(delR);

        // 4. Frequency-dependent width splitting
        if (m_params.freqWidth > 0.001f) {
            // Split low and high bands
            const float lowL = m_freqLpL2.process(m_freqLpL1.process(sigL));
            const float lowR = m_freqLpR2.process(m_freqLpR1.process(sigR));
            const float highL = m_freqHpL2.process(m_freqHpL1.process(sigL));
            const float highR = m_freqHpR2.process(m_freqHpR1.process(sigR));

            // Low band stays standard / narrow, High band gets expanded M/S width
            const float highMid = (highL + highR) * 0.5f;
            const float highSide = (highL - highR) * 0.5f * (1.0f + m_params.freqWidth * 1.5f);

            sigL = lowL + (highMid + highSide);
            sigR = lowR + (highMid - highSide);
        }

        // 5. Global Mid/Side Width control
        {
            const float mid = (sigL + sigR) * 0.5f;
            const float side = (sigL - sigR) * 0.5f * m_params.msWidth;
            sigL = mid + side;
            sigR = mid - side;
        }

        // 6. Stereo Rotation Matrix: [L', R'] = [L cos - R sin, L sin + R cos]
        if (std::abs(m_params.stereoRotationDeg) > 0.01f) {
            const float rotL = sigL * cosRot - sigR * sinRot;
            const float rotR = sigL * sinRot + sigR * cosRot;
            sigL = rotL;
            sigR = rotR;
        }

        // 7. 4th-order Linkwitz-Riley Bass Mono Collapse
        if (m_params.bassMonoCutoffHz > 10.0f) {
            const float lowL = m_bassLpL2.process(m_bassLpL1.process(sigL));
            const float lowR = m_bassLpR2.process(m_bassLpR1.process(sigR));
            const float monoLow = 0.5f * (lowL + lowR);

            const float highL = m_bassHpL2.process(m_bassHpL1.process(sigL));
            const float highR = m_bassHpR2.process(m_bassHpR1.process(sigR));

            sigL = monoLow + highL;
            sigR = monoLow + highR;
        }

        // 8. Dry/Wet Mix
        const float outL = lerp(inL, sigL, m_params.mix);
        const float outR = lerp(inR, sigR, m_params.mix);

        left[i] = outL;
        right[i] = outR;

        // 9. Real-time Phase Correlation & Metrics update
        const float prevL = m_corrBufferL[m_corrWritePos];
        const float prevR = m_corrBufferR[m_corrWritePos];

        m_sumL2 -= prevL * prevL;
        m_sumR2 -= prevR * prevR;
        m_sumLR -= prevL * prevR;

        m_corrBufferL[m_corrWritePos] = outL;
        m_corrBufferR[m_corrWritePos] = outR;

        m_sumL2 += outL * outL;
        m_sumR2 += outR * outR;
        m_sumLR += outL * outR;

        m_corrWritePos = (m_corrWritePos + 1) % m_corrWindowSize;
    }

    // Compute final phase correlation coefficient rho = sum(L*R) / sqrt(sum(L^2)*sum(R^2))
    const double denom = std::sqrt(std::max(1e-9, m_sumL2 * m_sumR2));
    m_metrics.phaseCorrelation = clamp(static_cast<float>(m_sumLR / denom), -1.0f, 1.0f);

    const double energySum = m_sumL2 + m_sumR2;
    if (energySum > 1e-9) {
        m_metrics.balance = clamp(static_cast<float>((m_sumR2 - m_sumL2) / energySum), -1.0f, 1.0f);
    } else {
        m_metrics.balance = 0.0f;
    }
}

} // namespace ff360
