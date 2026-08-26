#pragma once

#include "Common.h"
#include <memory>
#include <vector>
#include <array>

namespace ff360 {

enum class ReverbAlgorithmType {
    DigitalHall = 0,
    DarkPlate,
    GatedRoom,
    SynthRoom,
    Endless,
    Dream,
    Count
};

struct ReverbParameters {
    ReverbAlgorithmType algorithm = ReverbAlgorithmType::DigitalHall;
    float decayTime = 0.5f;        // 0.0 to 1.0 (maps to 0.2s to 30.0s / infinite)
    float preDelayMs = 20.0f;      // 0.0 to 250.0 ms
    float preDelayBpm = 120.0f;    // Host tempo for sync
    bool preDelaySync = false;     // Tempo-synced pre-delay (e.g. 1/16, 1/8 note)
    float ducking = 0.0f;          // 0.0 (no ducking) to 1.0 (heavy sidechain ducking)
    float width = 1.0f;            // 0.0 (mono) to 1.0 (wide)
    float modulation = 0.3f;       // 0.0 to 1.0 (chorus/pitch movement in tank)
    float lowDampingHz = 200.0f;   // 20 Hz to 1000 Hz
    float highDampingHz = 7500.0f; // 1000 Hz to 18000 Hz
    bool freeze = false;           // Infinite sustain toggle
    float mix = 0.35f;             // 0.0 to 1.0
};

// Pure abstract interface for algorithmic reverb tanks
class IReverbAlgorithm {
public:
    virtual ~IReverbAlgorithm() = default;
    virtual void prepare(double sampleRate, size_t maxBlockSize) = 0;
    virtual void reset() = 0;
    virtual void process(float inL, float inR, float decay, float mod, bool freeze, float& outL, float& outR) = 0;
};

// Allpass delay element for diffusion networks
class AllpassDelay {
public:
    void init(size_t delaySamples, float feedback = 0.7f) {
        m_buffer.assign(delaySamples + 4, 0.0f);
        m_delaySamples = delaySamples;
        m_feedback = feedback;
        m_index = 0;
    }

    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_index = 0;
    }

    inline float process(float input) noexcept {
        if (m_buffer.empty()) return input;
        const float bufOut = m_buffer[m_index];
        const float newBuf = input + bufOut * m_feedback;
        m_buffer[m_index] = newBuf;
        m_index = (m_index + 1) % m_delaySamples;
        return bufOut - input * m_feedback;
    }

private:
    std::vector<float> m_buffer;
    size_t m_delaySamples = 0;
    size_t m_index = 0;
    float m_feedback = 0.7f;
};

// Comb Filter with internal lowpass damping
class CombFilter {
public:
    void init(size_t delaySamples) {
        m_buffer.assign(delaySamples + 4, 0.0f);
        m_delaySamples = delaySamples;
        m_index = 0;
        m_filterStore = 0.0f;
    }

    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_index = 0;
        m_filterStore = 0.0f;
    }

    inline float process(float input, float feedback, float damp) noexcept {
        if (m_buffer.empty()) return input;
        const float output = m_buffer[m_index];
        m_filterStore = (output * (1.0f - damp)) + (m_filterStore * damp);
        m_buffer[m_index] = input + (m_filterStore * feedback);
        m_index = (m_index + 1) % m_delaySamples;
        return output;
    }

private:
    std::vector<float> m_buffer;
    size_t m_delaySamples = 0;
    size_t m_index = 0;
    float m_filterStore = 0.0f;
};

class FF360_DSP_ReverbEngine {
public:
    FF360_DSP_ReverbEngine();
    ~FF360_DSP_ReverbEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const ReverbParameters& params) noexcept;
    const ReverbParameters& getParameters() const noexcept { return m_params; }

    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    // keyLeft/keyRight optionally key the ducking envelope off an external sidechain
    // signal instead of the reverb's own input; pass nullptr for the previous
    // self-ducking-only behaviour.
    void processStereo(float* left, float* right, size_t numSamples,
                       const float* keyLeft = nullptr, const float* keyRight = nullptr);

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    ReverbParameters m_params;

    // Pre-delay lines
    FractionalDelayLine m_preDelayL;
    FractionalDelayLine m_preDelayR;

    // Algorithms
    std::array<std::unique_ptr<IReverbAlgorithm>, static_cast<size_t>(ReverbAlgorithmType::Count)> m_algorithms;

    // Damping filters
    BiquadFilter m_lowCutFilterL;
    BiquadFilter m_lowCutFilterR;
    BiquadFilter m_highCutFilterL;
    BiquadFilter m_highCutFilterR;

    // Fast envelope follower for instant ducking response
    float m_duckEnvelope = 0.0f;

    void createAlgorithms();
    void updateFilters();
};

} // namespace ff360
