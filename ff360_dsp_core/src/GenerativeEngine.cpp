#include "ff360/GenerativeEngine.h"
#include <algorithm>
#include <cmath>

namespace ff360 {

FF360_DSP_GenerativeEngine::FF360_DSP_GenerativeEngine() {
    reset();
}

void FF360_DSP_GenerativeEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    for (auto& pair : m_generators) {
        if (pair.second) pair.second->prepare(m_sampleRate);
    }
    reset();
}

void FF360_DSP_GenerativeEngine::reset() {
    m_isPlaying = false;
    m_currentSample = 0;
    m_progress = 0.0f;
    for (auto& pair : m_generators) {
        if (pair.second) pair.second->reset();
    }
}

void FF360_DSP_GenerativeEngine::registerGenerator(int id, std::shared_ptr<IGenerator> generator) {
    if (!generator) return;
    generator->prepare(m_sampleRate);
    m_generators[id] = generator;
    if (!m_activeGenerator || m_activeId == id) {
        m_activeGenerator = generator;
        m_activeId = id;
    }
}

void FF360_DSP_GenerativeEngine::selectGenerator(int id) {
    auto it = m_generators.find(id);
    if (it != m_generators.end()) {
        m_activeGenerator = it->second;
        m_activeId = id;
        m_activeGenerator->reset();
    }
}

float FF360_DSP_GenerativeEngine::calculateDurationSec(GenerativeSyncMode mode, float freeSec, float bpm) noexcept {
    const float validBpm = std::max(20.0f, bpm);
    const float beatSec = 60.0f / validBpm;
    const float barSec = beatSec * 4.0f;

    switch (mode) {
        case GenerativeSyncMode::FreeSeconds:  return std::max(0.1f, freeSec);
        case GenerativeSyncMode::Div_1_2_Bar:  return barSec * 0.5f;
        case GenerativeSyncMode::Div_1_Bar:    return barSec * 1.0f;
        case GenerativeSyncMode::Div_2_Bars:   return barSec * 2.0f;
        case GenerativeSyncMode::Div_4_Bars:   return barSec * 4.0f;
        case GenerativeSyncMode::Div_8_Bars:   return barSec * 8.0f;
    }
    return freeSec;
}

void FF360_DSP_GenerativeEngine::trigger(uint32_t seed) {
    m_params.seed = seed;
    const float durSec = calculateDurationSec(m_params.syncMode, m_params.freeDurationSec, m_params.hostBpm);
    m_totalDurationSamples = static_cast<size_t>(durSec * m_sampleRate);
    if (m_totalDurationSamples < 64) m_totalDurationSamples = 64;

    m_currentSample = 0;
    m_progress = 0.0f;
    m_isPlaying = true;

    if (m_activeGenerator) {
        m_activeGenerator->trigger(seed, durSec, m_params.intensity);
    }
}

void FF360_DSP_GenerativeEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
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

void FF360_DSP_GenerativeEngine::processStereo(float* left, float* right, size_t numSamples) {
    if (!m_activeGenerator) return;

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];

        float genL = 0.0f;
        float genR = 0.0f;

        if (m_isPlaying) {
            m_progress = static_cast<float>(m_currentSample) / static_cast<float>(m_totalDurationSamples);
            m_activeGenerator->processSample(genL, genR, m_progress);

            m_currentSample++;
            if (m_currentSample >= m_totalDurationSamples) {
                m_isPlaying = false;
                m_progress = 1.0f;
            }
        }

        left[i] = lerp(inL, inL + genL, m_params.mix);
        right[i] = lerp(inR, inR + genR, m_params.mix);
    }
}

} // namespace ff360
