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
    float sidechainSensitivity = 0.0f; // 0.0 (disabled) to 1.0 (fires on any sidechain transient)
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
    // keyLeft/keyRight, when provided with sidechainSensitivity > 0, force a fresh glitch
    // slice on every rising transient in the sidechain signal (e.g. "glitch on the kick").
    void processStereo(float* left, float* right, size_t numSamples,
                       const float* keyLeft = nullptr, const float* keyRight = nullptr);

    // --- UI-facing state for the live glitch-grid visualizer ---
    // Safe to poll from the message thread: all fields are plain scalars/fixed-size
    // arrays with no reallocation, matching the relaxed cross-thread read convention
    // used elsewhere in this DSP core (e.g. TapeStopController, StereoEngine metrics).
    static constexpr size_t kStepHistorySize = 16;

    // True while a captured slice is actively being played back this sample.
    bool isGlitching() const noexcept { return m_isGlitching; }

    // 0.0 to 1.0 progress through the current rhythm-grid subdivision, for a moving playhead.
    float getGridProgress() const noexcept {
        return m_sliceLengthSamples > 0
            ? 1.0f - (static_cast<float>(m_samplesUntilNextGrid) / static_cast<float>(m_sliceLengthSamples))
            : 0.0f;
    }

    // Whether each of the last kStepHistorySize grid steps actually triggered a glitch,
    // oldest first (index kStepHistorySize - 1 is the most recently evaluated step).
    std::array<bool, kStepHistorySize> getStepHistoryOldestFirst() const noexcept {
        std::array<bool, kStepHistorySize> out{};
        for (size_t i = 0; i < kStepHistorySize; ++i) {
            out[i] = m_stepHistory[(m_stepHistoryPos + i) % kStepHistorySize];
        }
        return out;
    }

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

    // Rolling history of recent grid-step trigger results, for the UI step-grid visualizer.
    std::array<bool, kStepHistorySize> m_stepHistory{};
    size_t m_stepHistoryPos = 0;

    // Sidechain transient detector state
    float m_scEnvelope = 0.0f;
    bool m_scWasAbove = false;

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
