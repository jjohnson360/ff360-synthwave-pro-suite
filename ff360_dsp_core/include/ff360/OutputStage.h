#pragma once

#include "Common.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace ff360 {

// Last stage of every Synthwave plugin: auto gain, output trim and click-free bypass.
//
//   captureDry()  at the top of processBlock, before the effect runs
//   process()     after the effect, before metering
//
// Auto gain matches the processed signal's loudness to the input's, so switching an effect in
// and out (or A/B-ing settings) compares character, not level. Both levels are measured with
// slow (1 s) mean-square ballistics; the correction is held while the input is silent (reverb
// tails, gaps) and limited to +/-12 dB. It's applied before the trim, so the trim still works.
//
// Bypass crossfades linearly to the untouched input over 10 ms. When the effect adds latency
// (oversampling), setDryDelay() delays the input by the same amount so dry and processed stay
// sample aligned. The effect keeps running while bypassed, so un-bypassing is click-free too.
class FF360_DSP_OutputStage {
public:
    struct Settings {
        float outputGainDb = 0.0f;
        bool autoGain = false;
        bool bypass = false;
    };

    void prepare(double sampleRate, size_t maxBlockSize) {
        m_sampleRate = static_cast<float>(sampleRate);
        m_dryL.assign(maxBlockSize, 0.0f);
        m_dryR.assign(maxBlockSize, 0.0f);
        m_bypassStep = 1.0f / std::max(1.0f, kBypassFadeSec * m_sampleRate);
        reset();
    }

    // Latency of the effect in samples; call while audio isn't running (prepare / suspended)
    void setDryDelay(size_t samples) {
        m_dryDelay = std::min(samples, kMaxDryDelay);
        std::fill(m_ringL.begin(), m_ringL.end(), 0.0f);
        std::fill(m_ringR.begin(), m_ringR.end(), 0.0f);
        m_ringWrite = 0;
    }

    void reset() {
        m_dryMs = m_wetMs = 0.0f;
        m_autoTargetDb = m_autoDb = 0.0f;
        m_gain = dbToGain(m_lastTrimDb);
        m_bypassMix = m_bypassTarget;
        m_appliedAutoGainDb.store(0.0f);
    }

    // Copies the input before the effect overwrites it. For mono, pass the same pointer twice.
    void captureDry(const float* left, const float* right, size_t numSamples) {
        if (numSamples > m_dryL.size()) { // host sent a bigger block than prepared (rare)
            m_dryL.resize(numSamples);
            m_dryR.resize(numSamples);
        }
        if (m_dryDelay == 0) {
            std::copy(left, left + numSamples, m_dryL.begin());
            std::copy(right, right + numSamples, m_dryR.begin());
        } else {
            for (size_t i = 0; i < numSamples; ++i) {
                m_ringL[m_ringWrite] = left[i];
                m_ringR[m_ringWrite] = right[i];
                const size_t read = (m_ringWrite + kRingSize - m_dryDelay) & (kRingSize - 1);
                m_dryL[i] = m_ringL[read];
                m_dryR[i] = m_ringR[read];
                m_ringWrite = (m_ringWrite + 1) & (kRingSize - 1);
            }
        }
        m_dryCount = numSamples;
    }

    void process(float* left, float* right, size_t numSamples, const Settings& s) {
        if (numSamples == 0) return;
        const bool mono = left == right;
        const size_t n = std::min(numSamples, m_dryCount);
        const float* dryL = m_dryL.data();
        const float* dryR = m_dryR.data();

        // ---- Loudness measurement (always runs, so switching Auto on is immediately right) ----
        float drySum = 0.0f, wetSum = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            drySum += dryL[i] * dryL[i] + dryR[i] * dryR[i];
            wetSum += left[i] * left[i] + right[i] * right[i];
        }
        const float blockDryMs = drySum / static_cast<float>(2 * std::max<size_t>(n, 1));
        const float blockWetMs = wetSum / static_cast<float>(2 * std::max<size_t>(n, 1));
        const float msCoeff = std::exp(-static_cast<float>(numSamples) / (kMeasureSec * m_sampleRate));
        m_dryMs = msCoeff * m_dryMs + (1.0f - msCoeff) * blockDryMs;
        m_wetMs = msCoeff * m_wetMs + (1.0f - msCoeff) * blockWetMs;

        // Only re-aim while there's input to match; hold through silence and tails
        if (m_dryMs > kSilenceMs && m_wetMs > kSilenceMs * 1.0e-3f)
            m_autoTargetDb = clamp(10.0f * std::log10(m_dryMs / m_wetMs), -kMaxAutoDb, kMaxAutoDb);

        const float autoAim = s.autoGain ? m_autoTargetDb : 0.0f;
        const float autoCoeff = std::exp(-static_cast<float>(numSamples) / (kAutoGlideSec * m_sampleRate));
        m_autoDb = autoCoeff * m_autoDb + (1.0f - autoCoeff) * autoAim;
        m_appliedAutoGainDb.store(s.autoGain ? m_autoDb : 0.0f);
        m_lastTrimDb = s.outputGainDb;

        // ---- Gain (auto + trim), ramped across the block ----
        const float g0 = m_gain;
        const float g1 = dbToGain(m_autoDb + s.outputGainDb);
        const float gStep = (g1 - g0) / static_cast<float>(numSamples);
        m_bypassTarget = s.bypass ? 1.0f : 0.0f;

        for (size_t i = 0; i < numSamples; ++i) {
            const float g = g0 + gStep * static_cast<float>(i + 1);

            if (m_bypassMix != m_bypassTarget) {
                m_bypassMix = m_bypassTarget > m_bypassMix ? std::min(m_bypassTarget, m_bypassMix + m_bypassStep)
                                                           : std::max(m_bypassTarget, m_bypassMix - m_bypassStep);
            }
            const float b = m_bypassMix;
            const float dl = i < n ? dryL[i] : 0.0f;
            const float dr = i < n ? dryR[i] : 0.0f;

            left[i] = left[i] * g * (1.0f - b) + dl * b;
            if (!mono)
                right[i] = right[i] * g * (1.0f - b) + dr * b;
        }
        m_gain = g1;
    }

    // Auto gain currently applied, in dB (0 when Auto is off). Safe to read from the UI thread.
    float getAppliedAutoGainDb() const noexcept { return m_appliedAutoGainDb.load(); }

private:
    static constexpr float kBypassFadeSec = 0.010f;
    static constexpr float kMeasureSec = 1.0f;     // loudness ballistics
    static constexpr float kAutoGlideSec = 0.050f; // how fast the applied gain follows its target
    static constexpr float kMaxAutoDb = 12.0f;
    static constexpr float kSilenceMs = 1.0e-6f;   // -60 dBFS RMS
    static constexpr size_t kRingSize = 4096;      // power of two
    static constexpr size_t kMaxDryDelay = kRingSize - 1;

    float m_sampleRate = 44100.0f;
    std::vector<float> m_dryL, m_dryR;
    size_t m_dryCount = 0;
    std::vector<float> m_ringL = std::vector<float>(kRingSize, 0.0f), m_ringR = std::vector<float>(kRingSize, 0.0f);
    size_t m_ringWrite = 0, m_dryDelay = 0;

    float m_dryMs = 0.0f, m_wetMs = 0.0f;
    float m_autoTargetDb = 0.0f, m_autoDb = 0.0f;
    float m_lastTrimDb = 0.0f;
    float m_gain = 1.0f;

    float m_bypassMix = 0.0f, m_bypassTarget = 0.0f, m_bypassStep = 0.01f;
    std::atomic<float> m_appliedAutoGainDb { 0.0f };
};

} // namespace ff360
