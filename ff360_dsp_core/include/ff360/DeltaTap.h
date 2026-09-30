#pragma once

#include "Common.h"
#include <algorithm>
#include <vector>

namespace ff360 {

// Delta listen: hear only what the effect adds.
//
//   capture()  the engine's input, right before the engine runs
//   apply()    right after it: output -= dryWeight * input
//
// Both calls must be at the engine's own rate, i.e. inside the oversampled domain when the
// plugin oversamples: the dry part of the output has then been through exactly the same path
// as the captured input and cancels exactly (subtracting after the IIR down-sampler would not).
//
// dryWeight picks what "added" means for the plugin, for engines that mix lerp(dry, wet, mix):
//   1          the difference, processed - dry (VHS, Glitch: the noise / distortion / glitches)
//   1 - mix    the wet signal alone at its mix level (Reverb, Chorus)
//
// Switching fades over 10 ms so it doesn't click.
class FF360_DSP_DeltaTap {
public:
    void prepare(double sampleRate, size_t maxBlockSize) {
        m_inL.assign(maxBlockSize, 0.0f);
        m_inR.assign(maxBlockSize, 0.0f);
        m_step = 1.0f / std::max(1.0f, 0.010f * static_cast<float>(sampleRate));
        m_amount = m_target;
    }

    // For mono, pass the same pointer twice
    void capture(const float* left, const float* right, size_t numSamples) {
        if (numSamples > m_inL.size()) { // bigger block than prepared (rare)
            m_inL.resize(numSamples);
            m_inR.resize(numSamples);
        }
        std::copy(left, left + numSamples, m_inL.begin());
        std::copy(right, right + numSamples, m_inR.begin());
    }

    void apply(float* left, float* right, size_t numSamples, bool on, float dryWeight) {
        m_target = on ? 1.0f : 0.0f;
        if (m_amount == 0.0f && m_target == 0.0f) return; // off: nothing to do
        const bool mono = left == right;
        for (size_t i = 0; i < numSamples; ++i) {
            if (m_amount != m_target)
                m_amount = m_target > m_amount ? std::min(m_target, m_amount + m_step)
                                               : std::max(m_target, m_amount - m_step);
            const float w = m_amount * dryWeight;
            left[i] -= w * m_inL[i];
            if (!mono)
                right[i] -= w * m_inR[i];
        }
    }

    bool isActive() const noexcept { return m_amount > 0.0f || m_target > 0.0f; }

private:
    std::vector<float> m_inL, m_inR;
    float m_amount = 0.0f, m_target = 0.0f, m_step = 0.01f;
};

} // namespace ff360
