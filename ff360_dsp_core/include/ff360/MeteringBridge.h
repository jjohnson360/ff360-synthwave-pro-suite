#pragma once

#include "Common.h"
#include <vector>
#include <array>
#include <cmath>

namespace ff360 {

struct MeterLevels {
    float peakL = MINUS_INFINITY_DB;
    float peakR = MINUS_INFINITY_DB;
    float truePeakL = MINUS_INFINITY_DB;
    float truePeakR = MINUS_INFINITY_DB;
    float rmsL = MINUS_INFINITY_DB;
    float rmsR = MINUS_INFINITY_DB;
    float vuL = MINUS_INFINITY_DB;
    float vuR = MINUS_INFINITY_DB;
    float loudnessLra = 0.0f; // Loudness range in LU
};

class FF360_DSP_MeteringBridge {
public:
    FF360_DSP_MeteringBridge();
    ~FF360_DSP_MeteringBridge() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset();

    void process(const float* const* inputs, size_t numChannels, size_t numSamples);
    void processStereo(const float* left, const float* right, size_t numSamples);

    MeterLevels getLevels() const noexcept { return m_levels; }

private:
    double m_sampleRate = 44100.0;
    MeterLevels m_levels;

    // RMS sliding window buffers
    std::vector<float> m_rmsBufferL;
    std::vector<float> m_rmsBufferR;
    size_t m_rmsWindowSize = 13230; // ~300ms window
    size_t m_rmsWritePos = 0;
    double m_rmsSumL = 0.0;
    double m_rmsSumR = 0.0;

    // VU meter ballistic filter state (IEC 60268-17: 300ms rise/fall)
    float m_vuStateL = 0.0f;
    float m_vuStateR = 0.0f;
    float m_vuCoeff = 0.01f;

    // True peak 4x polyphase intersample interpolation memory
    std::array<float, 4> m_tpHistoryL = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> m_tpHistoryR = { 0.0f, 0.0f, 0.0f, 0.0f };

    // LRA Gated buffer post Phase-10 rules (-70 dBFS absolute gate, -10 LU relative gate)
    std::vector<float> m_lraShortBlocks;
    size_t m_lraSampleCounter = 0;
    double m_lraBlockSum = 0.0;

    float calculateTruePeakSample(const float* history, float newSample) noexcept;
    void updateLra();
};

} // namespace ff360
