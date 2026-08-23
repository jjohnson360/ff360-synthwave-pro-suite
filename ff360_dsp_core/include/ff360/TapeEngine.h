#pragma once

#include "Common.h"
#include <array>
#include <vector>

namespace ff360 {

enum class TapeParamId : uint32_t {
    Saturation = 0,
    Wow,
    Flutter,
    PitchDrift,
    Noise,
    Hiss,
    Dropouts,
    LoFi,
    BitReduction,
    HighFrequencyLoss,
    StereoDrift,
    Warble,
    InputGain,
    OutputGain,
    Mix,
    Count
};

struct TapeParameters {
    float saturation = 0.0f;       // 0.0 - 1.0
    float wow = 0.0f;              // 0.0 - 1.0
    float flutter = 0.0f;          // 0.0 - 1.0
    float pitchDrift = 0.0f;       // 0.0 - 1.0
    float noise = 0.0f;            // 0.0 - 1.0
    float hiss = 0.0f;             // 0.0 - 1.0
    float dropouts = 0.0f;         // 0.0 - 1.0
    float loFi = 0.0f;             // 0.0 - 1.0
    float bitReduction = 0.0f;     // 0.0 - 1.0
    float highFrequencyLoss = 0.0f;// 0.0 - 1.0
    float stereoDrift = 0.0f;      // 0.0 - 1.0
    float warble = 0.0f;           // 0.0 - 1.0

    // Global levels
    float inputGainDb = 0.0f;      // -24 to +24 dB
    float outputGainDb = 0.0f;     // -24 to +24 dB
    float mix = 1.0f;              // 0.0 - 1.0
};

class FF360_DSP_TapeEngine {
public:
    FF360_DSP_TapeEngine();
    ~FF360_DSP_TapeEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const TapeParameters& params) noexcept;
    void setParameter(TapeParamId id, float normalizedValue) noexcept;
    float getParameter(TapeParamId id) const noexcept;

    // In-place or separate buffer stereo audio processing
    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    void processStereo(float* left, float* right, size_t numSamples);

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    TapeParameters m_params;

    // Fractional Delay lines for pitch modulation (wow, flutter, drift, warble, stereo drift)
    FractionalDelayLine m_delayLineL;
    FractionalDelayLine m_delayLineR;

    // Modulation oscillators / phases
    float m_wowPhase = 0.0f;
    float m_flutterPhase1 = 0.0f;
    float m_flutterPhase2 = 0.0f;
    float m_driftPhase = 0.0f;
    float m_warblePhaseL = 0.0f;
    float m_warblePhaseR = 0.0f;

    // High frequency loss filters
    BiquadFilter m_hfFilterL;
    BiquadFilter m_hfFilterR;

    // Noise and hiss generators & filters
    FastRandom m_rng;
    OnePoleFilter m_pinkFilterL;
    OnePoleFilter m_pinkFilterR;
    BiquadFilter m_hissFilterL;
    BiquadFilter m_hissFilterR;

    // Dropouts state
    float m_dropoutGainL = 1.0f;
    float m_dropoutGainR = 1.0f;
    float m_dropoutTimerL = 0.0f;
    float m_dropoutTimerR = 0.0f;

    // Lo-Fi sample rate reduction state
    float m_loFiPhase = 0.0f;
    float m_loFiHoldSampleL = 0.0f;
    float m_loFiHoldSampleR = 0.0f;

    // Saturation hysteresis memory
    float m_hysteresisStateL = 0.0f;
    float m_hysteresisStateR = 0.0f;

    void updateFilters();
};

} // namespace ff360
