#pragma once

#include "Common.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ff360 {

// Pluggable generator interface for transition and ambient generators
class IGenerator {
public:
    virtual ~IGenerator() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual void reset() = 0;
    virtual void trigger(uint32_t seed, float durationSec, float intensity) = 0;
    virtual void processSample(float& left, float& right, float progress) = 0;
    virtual const char* getName() const noexcept = 0;
};

enum class GenerativeSyncMode {
    FreeSeconds = 0,
    Div_1_2_Bar,
    Div_1_Bar,
    Div_2_Bars,
    Div_4_Bars,
    Div_8_Bars
};

struct GenerativeParameters {
    GenerativeSyncMode syncMode = GenerativeSyncMode::Div_2_Bars;
    float freeDurationSec = 2.0f;
    float hostBpm = 120.0f;
    float density = 0.5f;        // 0.0 to 1.0
    float evolveRate = 0.5f;     // 0.0 to 1.0
    uint32_t seed = 42;          // Recall seed
    float intensity = 0.8f;      // Macro / intensity
    float mix = 1.0f;
};

class FF360_DSP_GenerativeEngine {
public:
    FF360_DSP_GenerativeEngine();
    ~FF360_DSP_GenerativeEngine() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void registerGenerator(int id, std::shared_ptr<IGenerator> generator);
    void selectGenerator(int id);
    IGenerator* getActiveGenerator() const noexcept { return m_activeGenerator.get(); }

    void setParameters(const GenerativeParameters& params) noexcept { m_params = params; }
    const GenerativeParameters& getParameters() const noexcept { return m_params; }

    void trigger(uint32_t seed);
    void trigger() { trigger(m_params.seed); }

    bool isPlaying() const noexcept { return m_isPlaying; }
    float getProgress() const noexcept { return m_progress; }

    void process(const float* const* inputs, float* const* outputs, size_t numChannels, size_t numSamples);
    void processStereo(float* left, float* right, size_t numSamples);

    static float calculateDurationSec(GenerativeSyncMode mode, float freeSec, float bpm) noexcept;

private:
    double m_sampleRate = 44100.0;
    size_t m_maxBlockSize = 512;
    GenerativeParameters m_params;

    std::unordered_map<int, std::shared_ptr<IGenerator>> m_generators;
    std::shared_ptr<IGenerator> m_activeGenerator;
    int m_activeId = 0;

    bool m_isPlaying = false;
    size_t m_currentSample = 0;
    size_t m_totalDurationSamples = 44100;
    float m_progress = 0.0f;
};

} // namespace ff360
