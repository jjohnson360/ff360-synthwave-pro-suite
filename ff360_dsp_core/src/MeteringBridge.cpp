#include "ff360/MeteringBridge.h"
#include <cmath>
#include <algorithm>

namespace ff360 {

FF360_DSP_MeteringBridge::FF360_DSP_MeteringBridge() {
    reset();
}

void FF360_DSP_MeteringBridge::prepare(double sampleRate, size_t) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;

    // 300ms RMS sliding integration window
    m_rmsWindowSize = static_cast<size_t>(m_sampleRate * 0.3);
    if (m_rmsWindowSize < 16) m_rmsWindowSize = 16;
    m_rmsBufferL.assign(m_rmsWindowSize, 0.0f);
    m_rmsBufferR.assign(m_rmsWindowSize, 0.0f);

    // VU ballistic coefficient: 300ms rise time
    const float dt = 1.0f / static_cast<float>(m_sampleRate);
    m_vuCoeff = 1.0f - std::exp(-dt / 0.300f);

    reset();
}

void FF360_DSP_MeteringBridge::reset() {
    std::fill(m_rmsBufferL.begin(), m_rmsBufferL.end(), 0.0f);
    std::fill(m_rmsBufferR.begin(), m_rmsBufferR.end(), 0.0f);
    m_rmsWritePos = 0;
    m_rmsSumL = 0.0;
    m_rmsSumR = 0.0;

    m_vuStateL = 0.0f;
    m_vuStateR = 0.0f;

    m_tpHistoryL.fill(0.0f);
    m_tpHistoryR.fill(0.0f);

    m_lraShortBlocks.clear();
    m_lraShortBlocks.reserve(kMaxLraBlocks + 1);
    m_lraScratch.reserve(kMaxLraBlocks + 1);
    m_lraDirty = true;
    m_lraSampleCounter = 0;
    m_lraBlockSum = 0.0;

    m_levels = MeterLevels{};
}

float FF360_DSP_MeteringBridge::calculateTruePeakSample(const float* hist, float s) noexcept {
    // 4x polyphase Hermite interpolation to detect intersample peaks accurately
    const float y0 = hist[0];
    const float y1 = hist[1];
    const float y2 = hist[2];
    const float y3 = s;

    float maxVal = std::abs(y2);
    // Sub-sample evaluations at t = 0.25, 0.5, 0.75
    for (float frac : { 0.25f, 0.50f, 0.75f }) {
        const float interp = std::abs(cubicHermite(y0, y1, y2, y3, frac));
        if (interp > maxVal) maxVal = interp;
    }
    return maxVal;
}

void FF360_DSP_MeteringBridge::process(const float* const* inputs, size_t numChannels, size_t numSamples) {
    if (numChannels == 0 || numSamples == 0) return;

    if (numChannels == 1) {
        processStereo(inputs[0], inputs[0], numSamples);
    } else {
        processStereo(inputs[0], inputs[1], numSamples);
    }
}

void FF360_DSP_MeteringBridge::processStereo(const float* left, const float* right, size_t numSamples) {
    float peakL = 0.0f;
    float peakR = 0.0f;
    float truePeakL = 0.0f;
    float truePeakR = 0.0f;

    const size_t lraBlockSizeSamples = static_cast<size_t>(m_sampleRate * 0.1); // 100ms short block
    const double invRmsWindow = 1.0 / static_cast<double>(m_rmsWindowSize);

    for (size_t i = 0; i < numSamples; ++i) {
        const float sL = left[i];
        const float sR = right[i];

        const float absL = std::abs(sL);
        const float absR = std::abs(sR);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;

        // True peak 4x evaluation
        const float tpL = calculateTruePeakSample(m_tpHistoryL.data(), sL);
        const float tpR = calculateTruePeakSample(m_tpHistoryR.data(), sR);
        if (tpL > truePeakL) truePeakL = tpL;
        if (tpR > truePeakR) truePeakR = tpR;

        // Shift true peak history
        m_tpHistoryL[0] = m_tpHistoryL[1];
        m_tpHistoryL[1] = m_tpHistoryL[2];
        m_tpHistoryL[2] = sL;

        m_tpHistoryR[0] = m_tpHistoryR[1];
        m_tpHistoryR[1] = m_tpHistoryR[2];
        m_tpHistoryR[2] = sR;

        // RMS sliding sum
        const float sqL = sL * sL;
        const float sqR = sR * sR;
        m_rmsSumL -= m_rmsBufferL[m_rmsWritePos];
        m_rmsSumR -= m_rmsBufferR[m_rmsWritePos];
        m_rmsBufferL[m_rmsWritePos] = sqL;
        m_rmsBufferR[m_rmsWritePos] = sqR;
        m_rmsSumL += sqL;
        m_rmsSumR += sqR;
        m_rmsWritePos = (m_rmsWritePos + 1) % m_rmsWindowSize;

        // VU meter ballistic filter
        m_vuStateL += m_vuCoeff * (absL - m_vuStateL);
        m_vuStateR += m_vuCoeff * (absR - m_vuStateR);

        // LRA sample accumulation
        m_lraBlockSum += 0.5 * (sqL + sqR);
        m_lraSampleCounter++;
        if (m_lraSampleCounter >= lraBlockSizeSamples) {
            const float blockRms = static_cast<float>(std::sqrt(m_lraBlockSum / static_cast<double>(lraBlockSizeSamples)));
            const float blockDb = gainToDb(blockRms);
            // Absolute gate post Phase-10: -70 dBFS
            if (blockDb > -70.0f) {
                m_lraShortBlocks.push_back(blockDb);
                if (m_lraShortBlocks.size() > kMaxLraBlocks) { // keep last 60 seconds
                    m_lraShortBlocks.erase(m_lraShortBlocks.begin());
                }
                m_lraDirty = true;
            }
            m_lraBlockSum = 0.0;
            m_lraSampleCounter = 0;
        }
    }

    const float rmsL = static_cast<float>(std::sqrt(std::max(0.0, m_rmsSumL * invRmsWindow)));
    const float rmsR = static_cast<float>(std::sqrt(std::max(0.0, m_rmsSumR * invRmsWindow)));

    // Store levels in dBFS
    m_levels.peakL = gainToDb(peakL);
    m_levels.peakR = gainToDb(peakR);
    m_levels.truePeakL = gainToDb(truePeakL);
    m_levels.truePeakR = gainToDb(truePeakR);
    m_levels.rmsL = gainToDb(rmsL);
    m_levels.rmsR = gainToDb(rmsR);
    m_levels.vuL = gainToDb(m_vuStateL);
    m_levels.vuR = gainToDb(m_vuStateR);

    if (m_lraDirty) {
        updateLra();
        m_lraDirty = false;
    }
}

void FF360_DSP_MeteringBridge::updateLra() {
    if (m_lraShortBlocks.size() < 10) {
        m_levels.loudnessLra = 0.0f;
        return;
    }

    // Relative gate: -10 LU below integrated loudness
    double sum = 0.0;
    for (float db : m_lraShortBlocks) {
        sum += std::pow(10.0, db * 0.1);
    }
    const float meanDb = 10.0f * std::log10(static_cast<float>(sum / m_lraShortBlocks.size()));
    const float relativeGateThreshold = meanDb - 10.0f;

    auto& gatedBlocks = m_lraScratch; // reserved in prepare()
    gatedBlocks.clear();
    for (float db : m_lraShortBlocks) {
        if (db >= relativeGateThreshold) {
            gatedBlocks.push_back(db);
        }
    }

    if (gatedBlocks.size() < 4) {
        m_levels.loudnessLra = 0.0f;
        return;
    }

    std::sort(gatedBlocks.begin(), gatedBlocks.end());
    // 10th percentile to 95th percentile according to EBU R128 LRA definition
    const size_t idx10 = static_cast<size_t>(gatedBlocks.size() * 0.10f);
    const size_t idx95 = static_cast<size_t>(gatedBlocks.size() * 0.95f);
    m_levels.loudnessLra = std::max(0.0f, gatedBlocks[idx95] - gatedBlocks[idx10]);
}

} // namespace ff360
