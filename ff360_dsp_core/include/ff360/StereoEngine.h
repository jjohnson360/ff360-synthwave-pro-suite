#pragma once

#include "Common.h"
#include <vector>
#include <array>
#include <cmath>

namespace ff360 {

struct StereoParameters {
    float microDelayMs = 0.0f;       // 0.0 to 25.0 ms
    float haasWidth = 0.0f;          // 0.0 to 1.0 (Haas psychoacoustic Haas widening)
    float stereoDetune = 0.0f;       // 0.0 to 1.0 (subtle pitch separation)
    float msWidth = 1.0f;            // 0.0 (mono) to 2.0 (200% wide)
    float freqWidth = 0.0f;          // 0.0 to 1.0 (frequency-dependent width split)
    float freqWidthCrossoverHz = 500.0f; // Crossover for freq-dependent width
    float stereoRotationDeg = 0.0f;  // -90.0 to +90.0 degrees rotation
    float bassMonoCutoffHz = 120.0f; // 0 (off) to 400 Hz mono collapse
    float mix = 1.0f;                // 0.0 to 1.0
};

struct StereoMetrics {
    float phaseCorrelation = 1.0f;   // -1.0 (out of phase) to +1.0 (mono)
    float balance = 0.0f;            // -1.0 (Left) to +1.0 (Right)
    float stereoWidthFactor = 1.0f;  // Estimated energy width
};

class FF360_DSP_StereoEngine {
public:
    FF360_DSP_StereoEngine();
    ~FF360_DSP_StereoEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const StereoParameters& params) noexcept;
    const StereoParameters& getParameters() const noexcept { return m_params; }

    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    void processStereo(float* left, float* right, size_t numSamples);

    StereoMetrics getMetrics() const noexcept { return m_metrics; }

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    StereoParameters m_params;
    StereoMetrics m_metrics;

    // Fractional Delay lines for Micro-delay, Haas, and Detune
    FractionalDelayLine m_delayLineL;
    FractionalDelayLine m_delayLineR;

    // Detune LFO modulators
    float m_detunePhaseL = 0.0f;
    float m_detunePhaseR = 0.0f;

    // Bass Mono 4th-order Linkwitz-Riley filters
    BiquadFilter m_bassLpL1, m_bassLpL2;
    BiquadFilter m_bassLpR1, m_bassLpR2;
    BiquadFilter m_bassHpL1, m_bassHpL2;
    BiquadFilter m_bassHpR1, m_bassHpR2;

    // Freq-dependent width 4th-order Linkwitz-Riley filters
    BiquadFilter m_freqLpL1, m_freqLpL2;
    BiquadFilter m_freqLpR1, m_freqLpR2;
    BiquadFilter m_freqHpL1, m_freqHpL2;
    BiquadFilter m_freqHpR1, m_freqHpR2;

    // Correlation meter short sliding window
    std::vector<float> m_corrBufferL;
    std::vector<float> m_corrBufferR;
    size_t m_corrWritePos = 0;
    size_t m_corrWindowSize = 2048;
    double m_sumL2 = 0.0;
    double m_sumR2 = 0.0;
    double m_sumLR = 0.0;

    void updateFilters();
};

} // namespace ff360
