#include "ff360/GranularTexture.h"
#include <algorithm>
#include <cmath>

namespace ff360 {

FF360_DSP_GranularTexture::FF360_DSP_GranularTexture() {
    reset();
}

void FF360_DSP_GranularTexture::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // Generate rich synthwave analog harmonic source buffer (2 seconds long)
    const size_t sourceSamples = static_cast<size_t>(m_sampleRate * 2.0);
    m_sourceWaveform.resize(sourceSamples);

    const float baseFreq = 110.0f; // A2
    for (size_t i = 0; i < sourceSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(m_sampleRate);
        float s = 0.0f;
        // Multi-saw harmonics with subtle detuning
        s += 0.5f * std::sin(TWO_PI * baseFreq * t);
        s += 0.3f * std::sin(TWO_PI * (baseFreq * 2.0f + 0.15f) * t);
        s += 0.2f * std::sin(TWO_PI * (baseFreq * 3.0f - 0.2f) * t);
        s += 0.15f * std::sin(TWO_PI * (baseFreq * 4.0f + 0.35f) * t);
        s += 0.1f * std::sin(TWO_PI * (baseFreq * 5.0f) * t);
        m_sourceWaveform[i] = s * 0.5f;
    }

    reset();
}

void FF360_DSP_GranularTexture::reset() {
    for (auto& g : m_grains) {
        g.active = false;
    }
    m_spawnAccumulator = 0.0f;
    m_sourceReadPos = 0.0f;
}

void FF360_DSP_GranularTexture::setParameters(const GranularParameters& params) noexcept {
    m_params = params;
    if (params.seed != 0) {
        m_rng.setSeed(params.seed);
    }
}

void FF360_DSP_GranularTexture::spawnGrain() {
    if (m_sourceWaveform.empty()) return;

    for (auto& g : m_grains) {
        if (!g.active) {
            g.active = true;
            g.currentSample = 0.0f;

            // Grain length in samples
            const float lenMs = std::max(5.0f, m_params.grainSizeMs);
            g.lengthSamples = (lenMs * 0.001f) * static_cast<float>(m_sampleRate);

            // Position within source waveform
            const float posDrift = (m_rng.nextFloat() - 0.5f) * 0.2f * static_cast<float>(m_sourceWaveform.size());
            g.position = std::fmod(m_sourceReadPos + posDrift + static_cast<float>(m_sourceWaveform.size()), static_cast<float>(m_sourceWaveform.size()));

            // Pitch spray playback rate
            const float semitones = (m_rng.nextFloat() - 0.5f) * 2.0f * m_params.pitchSpraySemitones;
            g.playbackRate = std::pow(2.0f, semitones / 12.0f);

            // Stereo Pan
            const float pan = (m_rng.nextFloat() - 0.5f) * 2.0f * m_params.stereoSpread; // -1 to +1
            const float panRad = (pan + 1.0f) * 0.25f * PI; // 0 to pi/2
            g.panL = std::cos(panRad);
            g.panR = std::sin(panRad);

            break;
        }
    }
}

void FF360_DSP_GranularTexture::processStereo(float* left, float* right, size_t numSamples) {
    if (m_sourceWaveform.empty()) return;

    const float sr = static_cast<float>(m_sampleRate);
    const float spawnRate = std::max(0.1f, m_params.density) / sr;
    const size_t sourceSize = m_sourceWaveform.size();

    // Advance source position drift
    m_sourceReadPos += (m_params.evolveRate * 0.5f + 0.1f) * numSamples;
    if (m_sourceReadPos >= static_cast<float>(sourceSize)) {
        m_sourceReadPos = std::fmod(m_sourceReadPos, static_cast<float>(sourceSize));
    }

    for (size_t i = 0; i < numSamples; ++i) {
        // Grain spawn scheduler
        m_spawnAccumulator += spawnRate;
        if (m_spawnAccumulator >= 1.0f) {
            m_spawnAccumulator -= 1.0f;
            spawnGrain();
        }

        float grainSumL = 0.0f;
        float grainSumR = 0.0f;

        // Process active grains
        for (auto& g : m_grains) {
            if (!g.active) continue;

            // Hann window envelope: 0.5 * (1 - cos(2*pi*t))
            const float normPos = g.currentSample / g.lengthSamples;
            const float env = 0.5f * (1.0f - std::cos(TWO_PI * normPos));

            // Read source with linear interpolation
            const size_t idx0 = static_cast<size_t>(g.position) % sourceSize;
            const size_t idx1 = (idx0 + 1) % sourceSize;
            const float frac = g.position - std::floor(g.position);
            const float samp = lerp(m_sourceWaveform[idx0], m_sourceWaveform[idx1], frac) * env;

            grainSumL += samp * g.panL;
            grainSumR += samp * g.panR;

            // Advance grain
            g.position += g.playbackRate;
            if (g.position >= static_cast<float>(sourceSize)) g.position -= static_cast<float>(sourceSize);

            g.currentSample += 1.0f;
            if (g.currentSample >= g.lengthSamples) {
                g.active = false;
            }
        }

        left[i] = lerp(left[i], left[i] + grainSumL * 0.4f, m_params.mix);
        right[i] = lerp(right[i], right[i] + grainSumR * 0.4f, m_params.mix);
    }
}

} // namespace ff360
