#pragma once

#include "Common.h"
#include <vector>
#include <array>
#include <cmath>

namespace ff360 {

struct Grain {
    bool active = false;
    float position = 0.0f;
    float lengthSamples = 1000.0f;
    float playbackRate = 1.0f;
    float panL = 0.707f;
    float panR = 0.707f;
    float currentSample = 0.0f;
};

struct GranularParameters {
    float grainSizeMs = 80.0f;       // 10.0 to 300.0 ms
    float density = 25.0f;           // 1.0 to 60.0 grains per second
    float pitchSpraySemitones = 3.0f;// Random pitch deviation
    float stereoSpread = 0.8f;       // 0.0 to 1.0
    float evolveRate = 0.3f;         // Position drift rate
    uint32_t seed = 12345;
    float mix = 0.5f;
};

class FF360_DSP_GranularTexture {
public:
    FF360_DSP_GranularTexture();
    ~FF360_DSP_GranularTexture() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void setParameters(const GranularParameters& params) noexcept;
    const GranularParameters& getParameters() const noexcept { return m_params; }

    void processStereo(float* left, float* right, size_t numSamples);

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    GranularParameters m_params;

    // Internal synthetic waveform buffer for grain source (rich harmonic retro saw/square texture)
    std::vector<float> m_sourceWaveform;

    static constexpr size_t MAX_GRAINS = 48;
    std::array<Grain, MAX_GRAINS> m_grains;

    float m_spawnAccumulator = 0.0f;
    float m_sourceReadPos = 0.0f;
    FastRandom m_rng;

    void spawnGrain();
};

} // namespace ff360
