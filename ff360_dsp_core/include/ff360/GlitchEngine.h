#pragma once

#include "Common.h"
#include <vector>
#include <array>
#include <cmath>

namespace ff360 {

enum class GlitchTimingDivision {
    Div_1_4 = 0,
    Div_1_8,
    Div_1_16,
    Div_1_32
};

struct GlitchParameters {
    GlitchTimingDivision division = GlitchTimingDivision::Div_1_16;
    float hostBpm = 120.0f;
    float probability = 0.5f;        // 0.0 (never fire) to 1.0 (fire every grid beat)
    bool reverse = false;            // Reverse buffer playback
    bool freeze = false;             // Infinite slice freeze
    float pitchShiftSemitones = 0.0f;// -12.0 to +12.0 semitones
    float bitcrush = 0.0f;           // 0.0 to 1.0 (bit reduction)
    float gate = 1.0f;               // 0.1 to 1.0 (slice gate length / duty cycle)
    float filterCutoffHz = 20000.0f; // 200 Hz to 20000 Hz
    float filterResonance = 0.707f;  // 0.5 to 5.0 Q
    float mix = 0.5f;                // 0.0 to 1.0
};

class FF360_DSP_GlitchEngine {
public:
    FF360_DSP_GlitchEngine();
    ~FF360_DSP_GlitchEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const GlitchParameters& params) noexcept;
    const GlitchParameters& getParameters() const noexcept { return m_params; }

    void triggerManualGlitch();

    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    void processStereo(float* left, float* right, size_t numSamples);

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    GlitchParameters m_params;

    // Capture Ring Buffer (holds up to 4 seconds of audio)
    std::vector<float> m_captureBufferL;
    std::vector<float> m_captureBufferR;
    size_t m_writePos = 0;

    // Active Slice Playback State
    bool m_isGlitching = false;
    float m_playbackPos = 0.0f;
    size_t m_sliceLengthSamples = 2205; // ~1/16 note at 120bpm
    size_t m_sliceStartPos = 0;
    size_t m_samplesUntilNextGrid = 0;

    // Granular pitch rate
    float m_playbackRate = 1.0f;

    // Resonant Filter
    BiquadFilter m_filterL;
    BiquadFilter m_filterR;

    // PRNG
    FastRandom m_rng;

    size_t calculateSliceSamples() const noexcept;
    void evaluateGridTrigger();
};

} // namespace ff360
