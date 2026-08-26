#include "ff360/ReverbEngine.h"
#include <cmath>

namespace ff360 {

// -------------------------------------------------------------
// Algorithm 1: Digital Hall
// -------------------------------------------------------------
class DigitalHallAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        // 4 Input allpass diffusers
        m_diffusers[0].init(static_cast<size_t>(225 * srScale), 0.5f);
        m_diffusers[1].init(static_cast<size_t>(341 * srScale), 0.5f);
        m_diffusers[2].init(static_cast<size_t>(441 * srScale), 0.5f);
        m_diffusers[3].init(static_cast<size_t>(556 * srScale), 0.5f);

        // 8 Parallel combs
        static const size_t combLengths[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        for (size_t i = 0; i < 8; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }

        m_modPhase = 0.0f;
        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
        m_modPhase = 0.0f;
    }

    void process(float inL, float inR, float decay, float mod, bool freeze, float& outL, float& outR) override {
        float monoIn = (inL + inR) * 0.5f;

        // Diffusion stages
        for (auto& d : m_diffusers) {
            monoIn = d.process(monoIn);
        }

        const float feedback = freeze ? 0.999f : clamp(0.7f + decay * 0.28f, 0.0f, 0.98f);
        const float damp = freeze ? 0.0f : clamp(0.2f - decay * 0.1f, 0.02f, 0.8f);

        // Slow tank modulation
        m_modPhase += TWO_PI * 0.65f / static_cast<float>(m_sampleRate);
        if (m_modPhase >= TWO_PI) m_modPhase -= TWO_PI;
        const float modOffset = std::sin(m_modPhase) * (mod * 0.04f);

        float leftSum = 0.0f;
        float rightSum = 0.0f;

        for (size_t i = 0; i < 4; ++i) {
            leftSum += m_combs[i].process(monoIn, clamp(feedback + modOffset, 0.0f, 0.99f), damp);
        }
        for (size_t i = 4; i < 8; ++i) {
            rightSum += m_combs[i].process(monoIn, clamp(feedback - modOffset, 0.0f, 0.99f), damp);
        }

        outL = leftSum * 0.25f;
        outR = rightSum * 0.25f;
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 4> m_diffusers;
    std::array<CombFilter, 8> m_combs;
    float m_modPhase = 0.0f;
};

// -------------------------------------------------------------
// Algorithm 2: Dark Plate
// -------------------------------------------------------------
class DarkPlateAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        m_diffusers[0].init(static_cast<size_t>(142 * srScale), 0.75f);
        m_diffusers[1].init(static_cast<size_t>(107 * srScale), 0.75f);
        m_diffusers[2].init(static_cast<size_t>(379 * srScale), 0.625f);
        m_diffusers[3].init(static_cast<size_t>(277 * srScale), 0.625f);
        m_diffusers[4].init(static_cast<size_t>(672 * srScale), 0.625f);
        m_diffusers[5].init(static_cast<size_t>(908 * srScale), 0.625f);

        static const size_t combLengths[6] = { 1589, 1683, 1781, 1879, 2043, 2213 };
        for (size_t i = 0; i < 6; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }

        m_darkLpL.setType(OnePoleFilter::Type::Lowpass);
        m_darkLpR.setType(OnePoleFilter::Type::Lowpass);
        m_darkLpL.setCutoff(static_cast<float>(sampleRate), 4200.0f);
        m_darkLpR.setCutoff(static_cast<float>(sampleRate), 4200.0f);

        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
        m_darkLpL.reset();
        m_darkLpR.reset();
    }

    void process(float inL, float inR, float decay, float mod, bool freeze, float& outL, float& outR) override {
        float monoIn = (inL + inR) * 0.5f;

        for (auto& d : m_diffusers) {
            monoIn = d.process(monoIn);
        }

        const float feedback = freeze ? 0.999f : clamp(0.72f + decay * 0.25f, 0.0f, 0.97f);
        const float damp = freeze ? 0.0f : 0.45f;

        float leftSum = 0.0f;
        float rightSum = 0.0f;

        for (size_t i = 0; i < 3; ++i) {
            leftSum += m_combs[i].process(monoIn, feedback, damp);
        }
        for (size_t i = 3; i < 6; ++i) {
            rightSum += m_combs[i].process(monoIn, feedback, damp);
        }

        // Heavy high-frequency roll-off for classic dark plate character
        outL = m_darkLpL.process(leftSum * 0.33f);
        outR = m_darkLpR.process(rightSum * 0.33f);
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 6> m_diffusers;
    std::array<CombFilter, 6> m_combs;
    OnePoleFilter m_darkLpL;
    OnePoleFilter m_darkLpR;
};

// -------------------------------------------------------------
// Algorithm 3: Gated Room
// -------------------------------------------------------------
class GatedRoomAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        m_diffusers[0].init(static_cast<size_t>(180 * srScale), 0.7f);
        m_diffusers[1].init(static_cast<size_t>(290 * srScale), 0.7f);
        m_diffusers[2].init(static_cast<size_t>(410 * srScale), 0.6f);

        static const size_t combLengths[4] = { 850, 960, 1070, 1190 };
        for (size_t i = 0; i < 4; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }

        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
        m_gateTimer = 0.0f;
        m_envelope = 0.0f;
    }

    void process(float inL, float inR, float decay, float, bool freeze, float& outL, float& outR) override {
        const float inLevel = std::max(std::abs(inL), std::abs(inR));
        const float invSr = 1.0f / static_cast<float>(m_sampleRate);

        // Detect transient trigger
        if (inLevel > 0.05f) {
            m_gateTimer = lerp(0.12f, 0.45f, decay); // 120ms to 450ms gate duration
        }

        float monoIn = (inL + inR) * 0.5f;
        for (auto& d : m_diffusers) {
            monoIn = d.process(monoIn);
        }

        float leftSum = m_combs[0].process(monoIn, 0.88f, 0.1f) + m_combs[1].process(monoIn, 0.88f, 0.1f);
        float rightSum = m_combs[2].process(monoIn, 0.88f, 0.1f) + m_combs[3].process(monoIn, 0.88f, 0.1f);

        // Gate envelope: open full then abrupt cut
        if (freeze) {
            m_envelope = 1.0f;
        } else if (m_gateTimer > 0.0f) {
            m_gateTimer -= invSr;
            m_envelope = 1.0f;
        } else {
            // Fast exponential decay cutoff (classic non-linear gate clamp)
            m_envelope *= std::exp(-invSr * 120.0f);
        }

        outL = leftSum * 0.5f * m_envelope;
        outR = rightSum * 0.5f * m_envelope;
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 3> m_diffusers;
    std::array<CombFilter, 4> m_combs;
    float m_gateTimer = 0.0f;
    float m_envelope = 0.0f;
};

// -------------------------------------------------------------
// Algorithm 4: Synth Room
// -------------------------------------------------------------
class SynthRoomAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        m_diffusers[0].init(static_cast<size_t>(160 * srScale), 0.7f);
        m_diffusers[1].init(static_cast<size_t>(230 * srScale), 0.7f);

        static const size_t combLengths[4] = { 650, 780, 920, 1040 };
        for (size_t i = 0; i < 4; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }
        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
    }

    void process(float inL, float inR, float decay, float, bool freeze, float& outL, float& outR) override {
        float monoIn = (inL + inR) * 0.5f;
        for (auto& d : m_diffusers) monoIn = d.process(monoIn);

        const float feedback = freeze ? 0.999f : clamp(0.5f + decay * 0.4f, 0.0f, 0.92f);
        const float damp = freeze ? 0.0f : 0.25f;

        outL = (m_combs[0].process(monoIn, feedback, damp) + m_combs[1].process(monoIn, feedback, damp)) * 0.5f;
        outR = (m_combs[2].process(monoIn, feedback, damp) + m_combs[3].process(monoIn, feedback, damp)) * 0.5f;
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 2> m_diffusers;
    std::array<CombFilter, 4> m_combs;
};

// -------------------------------------------------------------
// Algorithm 5: Endless (Infinite Space)
// -------------------------------------------------------------
class EndlessAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        m_diffusers[0].init(static_cast<size_t>(340 * srScale), 0.65f);
        m_diffusers[1].init(static_cast<size_t>(520 * srScale), 0.65f);
        m_diffusers[2].init(static_cast<size_t>(780 * srScale), 0.65f);
        m_diffusers[3].init(static_cast<size_t>(1120 * srScale), 0.65f);

        static const size_t combLengths[8] = { 2130, 2480, 2790, 3120, 3450, 3810, 4200, 4650 };
        for (size_t i = 0; i < 8; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }
        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
    }

    void process(float inL, float inR, float decay, float, bool freeze, float& outL, float& outR) override {
        float monoIn = (inL + inR) * 0.5f;
        for (auto& d : m_diffusers) monoIn = d.process(monoIn);

        // Near-infinite feedback tank for cosmic endless tails
        const float feedback = freeze ? 0.9995f : clamp(0.85f + decay * 0.145f, 0.0f, 0.995f);
        const float damp = freeze ? 0.0f : clamp(0.12f - decay * 0.08f, 0.01f, 0.3f);

        float leftSum = 0.0f;
        float rightSum = 0.0f;
        for (size_t i = 0; i < 4; ++i) leftSum += m_combs[i].process(monoIn, feedback, damp);
        for (size_t i = 4; i < 8; ++i) rightSum += m_combs[i].process(monoIn, feedback, damp);

        outL = leftSum * 0.25f;
        outR = rightSum * 0.25f;
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 4> m_diffusers;
    std::array<CombFilter, 8> m_combs;
};

// -------------------------------------------------------------
// Algorithm 6: Dream (Modulated Shimmer Reverb)
// -------------------------------------------------------------
class DreamAlgorithm : public IReverbAlgorithm {
public:
    void prepare(double sampleRate, size_t) override {
        m_sampleRate = sampleRate;
        const float srScale = static_cast<float>(sampleRate / 44100.0);

        m_diffusers[0].init(static_cast<size_t>(250 * srScale), 0.6f);
        m_diffusers[1].init(static_cast<size_t>(480 * srScale), 0.6f);
        m_diffusers[2].init(static_cast<size_t>(830 * srScale), 0.6f);

        static const size_t combLengths[6] = { 1640, 1890, 2150, 2410, 2730, 3050 };
        for (size_t i = 0; i < 6; ++i) {
            m_combs[i].init(static_cast<size_t>(combLengths[i] * srScale));
        }

        m_shimmerDelayL.init(static_cast<size_t>(sampleRate * 0.1) + 64);
        m_shimmerDelayR.init(static_cast<size_t>(sampleRate * 0.1) + 64);
        m_modPhase1 = 0.0f;
        m_modPhase2 = 0.0f;
        reset();
    }

    void reset() override {
        for (auto& d : m_diffusers) d.reset();
        for (auto& c : m_combs) c.reset();
        m_shimmerDelayL.reset();
        m_shimmerDelayR.reset();
        m_modPhase1 = 0.0f;
        m_modPhase2 = 0.0f;
    }

    void process(float inL, float inR, float decay, float mod, bool freeze, float& outL, float& outR) override {
        float monoIn = (inL + inR) * 0.5f;
        for (auto& d : m_diffusers) monoIn = d.process(monoIn);

        const float feedback = freeze ? 0.999f : clamp(0.78f + decay * 0.2f, 0.0f, 0.98f);
        const float damp = freeze ? 0.0f : 0.18f;

        float leftSum = 0.0f;
        float rightSum = 0.0f;
        for (size_t i = 0; i < 3; ++i) leftSum += m_combs[i].process(monoIn, feedback, damp);
        for (size_t i = 3; i < 6; ++i) rightSum += m_combs[i].process(monoIn, feedback, damp);

        leftSum *= 0.33f;
        rightSum *= 0.33f;

        // Dream modulated pitch drift & octave-sheen shimmer simulation
        const float invSr = 1.0f / static_cast<float>(m_sampleRate);
        m_modPhase1 += TWO_PI * 1.8f * invSr;
        m_modPhase2 += TWO_PI * 2.3f * invSr;
        if (m_modPhase1 >= TWO_PI) m_modPhase1 -= TWO_PI;
        if (m_modPhase2 >= TWO_PI) m_modPhase2 -= TWO_PI;

        m_shimmerDelayL.write(leftSum);
        m_shimmerDelayR.write(rightSum);

        const float centerDelay = 200.0f;
        const float shimmerModL = std::sin(m_modPhase1) * (15.0f + mod * 40.0f);
        const float shimmerModR = std::cos(m_modPhase2) * (15.0f + mod * 40.0f);

        const float pitchSheenL = m_shimmerDelayL.readFractional(centerDelay + shimmerModL);
        const float pitchSheenR = m_shimmerDelayR.readFractional(centerDelay + shimmerModR);

        outL = leftSum + pitchSheenL * (0.35f * (mod + 0.3f));
        outR = rightSum + pitchSheenR * (0.35f * (mod + 0.3f));
    }

private:
    double m_sampleRate = 44100.0;
    std::array<AllpassDelay, 3> m_diffusers;
    std::array<CombFilter, 6> m_combs;
    FractionalDelayLine m_shimmerDelayL;
    FractionalDelayLine m_shimmerDelayR;
    float m_modPhase1 = 0.0f;
    float m_modPhase2 = 0.0f;
};

// -------------------------------------------------------------
// FF360_DSP_ReverbEngine Core Pipeline
// -------------------------------------------------------------
FF360_DSP_ReverbEngine::FF360_DSP_ReverbEngine() {
    createAlgorithms();
    reset();
}

void FF360_DSP_ReverbEngine::createAlgorithms() {
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::DigitalHall)] = std::make_unique<DigitalHallAlgorithm>();
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::DarkPlate)] = std::make_unique<DarkPlateAlgorithm>();
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::GatedRoom)] = std::make_unique<GatedRoomAlgorithm>();
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::SynthRoom)] = std::make_unique<SynthRoomAlgorithm>();
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::Endless)] = std::make_unique<EndlessAlgorithm>();
    m_algorithms[static_cast<size_t>(ReverbAlgorithmType::Dream)] = std::make_unique<DreamAlgorithm>();
}

void FF360_DSP_ReverbEngine::prepare(double sampleRate, size_t maxBlockSize) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    m_maxBlockSize = maxBlockSize;

    // 500ms max pre-delay buffer
    const size_t maxPreDelaySamples = static_cast<size_t>(m_sampleRate * 0.5) + 64;
    m_preDelayL.init(maxPreDelaySamples);
    m_preDelayR.init(maxPreDelaySamples);

    for (auto& alg : m_algorithms) {
        if (alg) alg->prepare(m_sampleRate, maxBlockSize);
    }

    updateFilters();
    reset();
}

void FF360_DSP_ReverbEngine::reset() {
    m_preDelayL.reset();
    m_preDelayR.reset();
    for (auto& alg : m_algorithms) {
        if (alg) alg->reset();
    }
    m_lowCutFilterL.reset();
    m_lowCutFilterR.reset();
    m_highCutFilterL.reset();
    m_highCutFilterR.reset();
    m_duckEnvelope = 0.0f;
}

void FF360_DSP_ReverbEngine::setParameters(const ReverbParameters& params) noexcept {
    m_params = params;
    updateFilters();
}

void FF360_DSP_ReverbEngine::updateFilters() {
    const float sr = static_cast<float>(m_sampleRate);
    m_lowCutFilterL.configure(BiquadFilter::Type::Highpass, sr, std::max(20.0f, m_params.lowDampingHz), 0.707f);
    m_lowCutFilterR.configure(BiquadFilter::Type::Highpass, sr, std::max(20.0f, m_params.lowDampingHz), 0.707f);

    m_highCutFilterL.configure(BiquadFilter::Type::Lowpass, sr, std::min(sr * 0.48f, m_params.highDampingHz), 0.707f);
    m_highCutFilterR.configure(BiquadFilter::Type::Lowpass, sr, std::min(sr * 0.48f, m_params.highDampingHz), 0.707f);
}

void FF360_DSP_ReverbEngine::process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples) {
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

void FF360_DSP_ReverbEngine::processStereo(float* left, float* right, size_t numSamples,
                                            const float* keyLeft, const float* keyRight) {
    const float sr = static_cast<float>(m_sampleRate);
    const float invSr = 1.0f / sr;

    // Calculate effective pre-delay time
    float preDelaySamples = 0.0f;
    if (m_params.preDelaySync && m_params.preDelayBpm > 20.0f) {
        // 1/16th note sync: (60.0 / bpm) * 0.25
        const float beatSec = (60.0f / m_params.preDelayBpm) * 0.25f;
        preDelaySamples = beatSec * sr;
    } else {
        preDelaySamples = (m_params.preDelayMs * 0.001f) * sr;
    }
    preDelaySamples = clamp(preDelaySamples, 0.0f, static_cast<float>(sr * 0.5));

    const size_t algIndex = static_cast<size_t>(m_params.algorithm) % static_cast<size_t>(ReverbAlgorithmType::Count);
    auto* algorithm = m_algorithms[algIndex].get();
    if (!algorithm) return;

    const float decay = m_params.decayTime;
    const float mod = m_params.modulation;
    const bool freeze = m_params.freeze;
    const float width = m_params.width;
    const float ducking = m_params.ducking;
    const float mix = m_params.mix;

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];

        // 1. Instant Ducking detector: fast attack (within buffer), release ~150ms.
        // Keyed off the external sidechain input when one is connected, otherwise
        // off the reverb's own dry input (self-ducking, the original behaviour).
        const float keyL = keyLeft ? keyLeft[i] : inL;
        const float keyR = keyRight ? keyRight[i] : inR;
        const float inAbs = std::max(std::abs(keyL), std::abs(keyR));
        if (inAbs > m_duckEnvelope) {
            m_duckEnvelope += (inAbs - m_duckEnvelope) * 0.5f; // very fast attack
        } else {
            m_duckEnvelope += (inAbs - m_duckEnvelope) * (invSr * 12.0f); // ~80-150ms release
        }

        // Calculate ducking attenuation factor
        const float duckGain = clamp(1.0f - (m_duckEnvelope * ducking * 1.5f), 0.0f, 1.0f);

        // 2. Pre-delay
        m_preDelayL.write(inL);
        m_preDelayR.write(inR);

        const float delayedInL = m_preDelayL.readFractional(preDelaySamples);
        const float delayedInR = m_preDelayR.readFractional(preDelaySamples);

        // 3. Algorithmic Reverb Tank
        float revL = 0.0f;
        float revR = 0.0f;
        algorithm->process(delayedInL, delayedInR, decay, mod, freeze, revL, revR);

        // 4. Low & High Damping / EQ Filters
        revL = m_lowCutFilterL.process(revL);
        revR = m_lowCutFilterR.process(revR);
        revL = m_highCutFilterL.process(revL);
        revR = m_highCutFilterR.process(revR);

        // 5. Width Control (M/S adjustment)
        const float mid = (revL + revR) * 0.5f;
        const float side = (revL - revR) * 0.5f * width;
        revL = (mid + side) * duckGain;
        revR = (mid - side) * duckGain;

        // 6. Dry/Wet Mix
        left[i] = lerp(inL, revL, mix);
        right[i] = lerp(inR, revR, mix);
    }
}

} // namespace ff360
