#pragma once

#include "GenerativeEngine.h"
#include "TapeEngine.h"
#include "TapeStopController.h"
#include "Common.h"
#include <cmath>
#include <vector>

namespace ff360 {

// 1. Noise Sweep Generator
class NoiseSweepGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override {
        m_sampleRate = sampleRate;
        m_filterL.configure(BiquadFilter::Type::Bandpass, static_cast<float>(sampleRate), 1000.0f, 3.0f);
        m_filterR.configure(BiquadFilter::Type::Bandpass, static_cast<float>(sampleRate), 1000.0f, 3.0f);
    }
    void reset() override { m_filterL.reset(); m_filterR.reset(); }
    void trigger(uint32_t seed, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_rng.setSeed(seed);
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        // Cutoff sweeps from 200 Hz to 16 kHz with exponential curve, transposed by the pitch range
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float cutoff = 200.0f * std::pow(80.0f, progress) * pitchMult;
        const float sr = static_cast<float>(m_sampleRate);
        m_filterL.configure(BiquadFilter::Type::Bandpass, sr, std::min(cutoff, sr * 0.45f), 2.5f + progress * 2.0f);
        m_filterR.configure(BiquadFilter::Type::Bandpass, sr, std::min(cutoff * 1.05f, sr * 0.45f), 2.5f + progress * 2.0f);

        const float noiseL = (m_rng.nextFloat() * 2.0f - 1.0f);
        const float noiseR = (m_rng.nextFloat() * 2.0f - 1.0f);
        const float env = std::pow(progress, 2.0f) * m_intensity;

        left = m_filterL.process(noiseL) * env * 2.5f;
        right = m_filterR.process(noiseR) * env * 2.5f;
    }
    const char* getName() const noexcept override { return "Noise Sweep"; }
private:
    double m_sampleRate = 44100.0;
    BiquadFilter m_filterL, m_filterR;
    FastRandom m_rng;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 2. Pitch Sweep Generator
class PitchSweepGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override { m_sampleRate = sampleRate; }
    void reset() override { m_phase = 0.0f; }
    void trigger(uint32_t /*seed*/, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float freq = 100.0f * std::pow(20.0f, progress) * pitchMult;
        m_phase += TWO_PI * freq / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        const float env = std::sin(progress * PI) * m_intensity;
        const float s = std::sin(m_phase) * env;
        left = s;
        right = s;
    }
    const char* getName() const noexcept override { return "Pitch Sweep"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 3. Laser Generator
class LaserGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override { m_sampleRate = sampleRate; }
    void reset() override { m_phase = 0.0f; }
    void trigger(uint32_t /*seed*/, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        // Fast pitch drop: 3000 Hz down to 80 Hz, transposed by the pitch range
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float freq = (80.0f + 3000.0f * std::pow(1.0f - progress, 4.0f)) * pitchMult;
        m_phase += TWO_PI * freq / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        const float env = (1.0f - progress) * m_intensity;
        const float s = std::sin(m_phase) * env * 0.9f;
        left = s;
        right = s;
    }
    const char* getName() const noexcept override { return "Laser"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 4. Reverse Generator
class ReverseGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override {
        m_sampleRate = sampleRate;
        m_filter.configure(BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 4000.0f, 1.0f);
    }
    void reset() override { m_phase = 0.0f; m_filter.reset(); }
    void trigger(uint32_t seed, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_rng.setSeed(seed);
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        // Reverse exponential swell, transposed by the pitch range
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float env = std::pow(progress, 3.5f) * m_intensity;
        const float n = (m_rng.nextFloat() * 2.0f - 1.0f) * 0.6f;
        m_phase += TWO_PI * (150.0f + progress * 400.0f) * pitchMult / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        const float sig = m_filter.process(n + std::sin(m_phase) * 0.4f) * env;
        left = sig;
        right = sig;
    }
    const char* getName() const noexcept override { return "Reverse"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    BiquadFilter m_filter;
    FastRandom m_rng;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 5. Impact Generator
class ImpactGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override { m_sampleRate = sampleRate; }
    void reset() override { m_phase = 0.0f; }
    void trigger(uint32_t seed, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_rng.setSeed(seed);
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        // Low-end sub drop + noise burst, transposed by the pitch range
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float subFreq = (40.0f + 120.0f * (1.0f - progress)) * pitchMult;
        m_phase += TWO_PI * subFreq / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        const float env = std::exp(-progress * 5.0f) * m_intensity;
        const float noise = (m_rng.nextFloat() * 2.0f - 1.0f) * std::exp(-progress * 25.0f) * 0.5f;
        const float s = (std::sin(m_phase) * 0.8f + noise) * env;
        left = s;
        right = s;
    }
    const char* getName() const noexcept override { return "Impact"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    FastRandom m_rng;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 6. Riser Generator
class RiserGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override { m_sampleRate = sampleRate; }
    void reset() override { m_phase1 = 0.0f; m_phase2 = 0.0f; }
    void trigger(uint32_t /*seed*/, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float f1 = 220.0f * std::pow(4.0f, progress) * pitchMult;
        const float f2 = f1 * 1.01f; // Detuned twin

        m_phase1 += TWO_PI * f1 / static_cast<float>(m_sampleRate);
        m_phase2 += TWO_PI * f2 / static_cast<float>(m_sampleRate);
        if (m_phase1 >= TWO_PI) m_phase1 -= TWO_PI;
        if (m_phase2 >= TWO_PI) m_phase2 -= TWO_PI;

        const float env = std::pow(progress, 1.8f) * m_intensity;
        left = std::sin(m_phase1) * env * 0.7f;
        right = std::sin(m_phase2) * env * 0.7f;
    }
    const char* getName() const noexcept override { return "Riser"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase1 = 0.0f;
    float m_phase2 = 0.0f;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 7. Downlifter Generator
class DownlifterGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override {
        m_sampleRate = sampleRate;
        m_filter.configure(BiquadFilter::Type::Lowpass, static_cast<float>(sampleRate), 12000.0f, 2.0f);
    }
    void reset() override { m_phase = 0.0f; m_filter.reset(); }
    void trigger(uint32_t seed, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_rng.setSeed(seed);
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const float cutoff = 12000.0f * (1.0f - progress * 0.9f);
        m_filter.configure(BiquadFilter::Type::Lowpass, static_cast<float>(m_sampleRate), std::max(60.0f, cutoff), 2.0f);

        const float subFreq = 160.0f * (1.0f - progress * 0.75f) * pitchMult;
        m_phase += TWO_PI * subFreq / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        const float noise = (m_rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
        const float env = (1.0f - progress) * m_intensity;
        const float s = m_filter.process(std::sin(m_phase) * 0.7f + noise) * env;
        left = s;
        right = s;
    }
    const char* getName() const noexcept override { return "Downlifter"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    BiquadFilter m_filter;
    FastRandom m_rng;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 8. Digital Sweep Generator
class DigitalSweepGenerator : public IGenerator {
public:
    void prepare(double sampleRate) override { m_sampleRate = sampleRate; }
    void reset() override { m_phase = 0.0f; }
    void trigger(uint32_t /*seed*/, float /*durSec*/, float intensity,
                float startSemitones, float endSemitones) override {
        m_intensity = intensity;
        m_pitchMultStart = std::pow(2.0f, startSemitones / 12.0f);
        m_pitchMultEnd = std::pow(2.0f, endSemitones / 12.0f);
        reset();
    }
    void processSample(float& left, float& right, float progress) override {
        // Stepped 8-bit retro arpeggio notes, transposed by the pitch range
        const float pitchMult = lerp(m_pitchMultStart, m_pitchMultEnd, progress);
        const int noteStep = static_cast<int>(progress * 24.0f); // 2 octaves in 24 steps
        const float f = 110.0f * std::pow(1.059463f, static_cast<float>(noteStep)) * pitchMult;

        m_phase += TWO_PI * f / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        // Square wave
        float s = (m_phase < PI) ? 0.7f : -0.7f;
        const float env = progress * m_intensity;
        s *= env;

        left = s;
        right = s;
    }
    const char* getName() const noexcept override { return "Digital Sweep"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    float m_intensity = 1.0f;
    float m_pitchMultStart = 1.0f;
    float m_pitchMultEnd = 1.0f;
};

// 9. Tape Sweep Generator (Reuses TapeStopController & TapeEngine)
class TapeSweepGenerator : public IGenerator {
public:
    TapeSweepGenerator() = default;
    void prepare(double sampleRate) override {
        m_sampleRate = sampleRate;
        m_tapeEngine.prepare(sampleRate, 512);
        m_controller.prepare(sampleRate);
    }
    void reset() override {
        m_phase = 0.0f;
        m_tapeEngine.reset();
        m_controller.reset();
    }
    void trigger(uint32_t /*seed*/, float durSec, float intensity,
                float startSemitones, float /*endSemitones*/) override {
        m_intensity = intensity;
        m_durationSec = durSec;
        // The tape-stop slowdown already sweeps pitch downward on its own; startSemitones
        // just sets the carrier's starting register (end is governed by the stop curve).
        m_carrierMult = std::pow(2.0f, startSemitones / 12.0f);
        reset();

        TapeStopParameters p;
        p.slowdownTimeSec = durSec * 0.9f;
        p.profile = TapeStopProfile::TapeStop;
        p.pitchCurve = 0.6f;
        p.filterMovement = 0.8f;
        m_controller.setParameters(p);
        m_controller.triggerStop();
    }
    void processSample(float& left, float& right, float /*progress*/) override {
        // Oscillator carrier source through tape engine slowdown
        m_phase += TWO_PI * 440.0f * m_carrierMult / static_cast<float>(m_sampleRate);
        if (m_phase >= TWO_PI) m_phase -= TWO_PI;

        m_controller.updateAndApply(m_tapeEngine, 1);

        float sL = std::sin(m_phase) * 0.6f;
        float sR = sL;
        m_tapeEngine.processStereo(&sL, &sR, 1);

        left = sL * m_intensity;
        right = sR * m_intensity;
    }
    const char* getName() const noexcept override { return "Tape Sweep"; }
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    float m_durationSec = 2.0f;
    float m_intensity = 1.0f;
    float m_carrierMult = 1.0f;
    FF360_DSP_TapeEngine m_tapeEngine;
    FF360_DSP_TapeStopController m_controller;
};

} // namespace ff360
