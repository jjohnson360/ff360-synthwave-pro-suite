#pragma once

#include "Common.h"
#include <vector>
#include <array>

namespace ff360 {

enum class ChorusMode {
    Stereo2Voice = 0,
    Quad4Voice = 1
};

enum class ChorusCharacter {
    Modern = 0,
    Vintage = 1
};

struct ModulationParameters {
    float rateHz = 0.8f;           // 0.05 Hz to 8.0 Hz
    float depth = 0.5f;            // 0.0 to 1.0 (modulation excursion)
    float width = 1.0f;            // 0.0 (mono) to 1.0 (ultra-wide stereo)
    float detune = 0.3f;           // 0.0 to 1.0 (pitch spread offset between voices)
    float feedback = 0.0f;         // 0.0 to 0.95 (flanging/resonance)
    float preDelayMs = 4.0f;       // 0.0 to 30.0 ms
    float mix = 0.5f;              // 0.0 to 1.0
    float bassMonoCrossoverHz = 140.0f; // Mono cutoff for low end (0 = off, up to 300 Hz)
    ChorusMode mode = ChorusMode::Stereo2Voice;
    ChorusCharacter character = ChorusCharacter::Vintage;
};

class FF360_DSP_ModulationEngine {
public:
    FF360_DSP_ModulationEngine();
    ~FF360_DSP_ModulationEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const ModulationParameters& params) noexcept;
    const ModulationParameters& getParameters() const noexcept { return m_params; }

    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    void processStereo(float* left, float* right, size_t numSamples);

private:
    static constexpr size_t MAX_VOICES = 4;

    struct Voice {
        FractionalDelayLine delayLine;
        float lfoPhase = 0.0f;
        float phaseOffset = 0.0f;
        float panL = 0.5f;
        float panR = 0.5f;
        float feedbackSample = 0.0f;
        float driftPhase = 0.0f;
    };

    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    ModulationParameters m_params;

    std::array<Voice, MAX_VOICES> m_voices;

    // Bass Mono Crossover Filters (4th order Linkwitz-Riley: 2 cascaded Butterworth pairs)
    BiquadFilter m_bassLpL1, m_bassLpL2;
    BiquadFilter m_bassLpR1, m_bassLpR2;
    BiquadFilter m_bassHpL1, m_bassHpL2;
    BiquadFilter m_bassHpR1, m_bassHpR2;

    // Vintage character analog warmth filters
    OnePoleFilter m_vintageLpL;
    OnePoleFilter m_vintageLpR;

    FastRandom m_rng;

    void setupVoicePans();
};

} // namespace ff360
