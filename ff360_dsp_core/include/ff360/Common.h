#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include "Preset.h"

namespace ff360 {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float MINUS_INFINITY_DB = -100.0f;

inline float clamp(float value, float minVal, float maxVal) noexcept {
    return std::max(minVal, std::min(maxVal, value));
}

inline float dbToGain(float db) noexcept {
    if (db <= MINUS_INFINITY_DB) return 0.0f;
    return std::pow(10.0f, db * 0.05f);
}

inline float gainToDb(float gain) noexcept {
    if (gain <= 0.00001f) return MINUS_INFINITY_DB;
    return 20.0f * std::log10(gain);
}

inline float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

// Fast cubic hermite interpolation for fractional delay lines
inline float cubicHermite(float y0, float y1, float y2, float y3, float frac) noexcept {
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// Fractional Delay Line
class FractionalDelayLine {
public:
    FractionalDelayLine() = default;
    
    void init(size_t maxDelaySamples) {
        m_buffer.assign(maxDelaySamples + 4, 0.0f);
        m_writePos = 0;
        m_maxDelay = maxDelaySamples;
    }

    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
    }

    inline void write(float sample) noexcept {
        m_buffer[m_writePos] = sample;
        m_writePos = (m_writePos + 1) % m_buffer.size();
    }

    inline float readFractional(float delaySamples) const noexcept {
        if (m_buffer.empty()) return 0.0f;
        
        delaySamples = clamp(delaySamples, 0.0f, static_cast<float>(m_maxDelay));
        // m_writePos points to the NEXT write position, so the last written sample is at (m_writePos - 1)
        float readPos = static_cast<float>(m_writePos) - 1.0f - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(m_buffer.size());
        
        const size_t i1 = static_cast<size_t>(readPos) % m_buffer.size();
        const float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));
        
        const size_t i0 = (i1 + m_buffer.size() - 1) % m_buffer.size();
        const size_t i2 = (i1 + 1) % m_buffer.size();
        const size_t i3 = (i1 + 2) % m_buffer.size();

        return cubicHermite(m_buffer[i0], m_buffer[i1], m_buffer[i2], m_buffer[i3], frac);
    }

private:
    std::vector<float> m_buffer;
    size_t m_writePos = 0;
    size_t m_maxDelay = 0;
};

// One-Pole Lowpass / Highpass Filter
class OnePoleFilter {
public:
    enum class Type { Lowpass, Highpass };

    void setType(Type type) noexcept { m_type = type; }

    void setCutoff(float sampleRate, float cutoffHz) noexcept {
        if (sampleRate <= 0.0f) return;
        cutoffHz = clamp(cutoffHz, 10.0f, sampleRate * 0.49f);
        const float x = TWO_PI * cutoffHz / sampleRate;
        m_a0 = 1.0f - std::exp(-x);
    }

    void reset() noexcept {
        m_z1 = 0.0f;
    }

    inline float process(float input) noexcept {
        m_z1 += m_a0 * (input - m_z1);
        if (m_type == Type::Highpass) {
            return input - m_z1;
        }
        return m_z1;
    }

private:
    Type m_type = Type::Lowpass;
    float m_a0 = 0.1f;
    float m_z1 = 0.0f;
};

// Biquad Filter (Lowpass, Highpass, Bandpass, Peaking, High Shelf, Low Shelf)
class BiquadFilter {
public:
    enum class Type { Lowpass, Highpass, Bandpass, HighShelf, LowShelf };

    void configure(Type type, float sampleRate, float frequency, float q = 0.7071f, float gainDb = 0.0f) noexcept {
        if (sampleRate <= 0.0f) return;
        frequency = clamp(frequency, 10.0f, sampleRate * 0.49f);
        const float w0 = TWO_PI * frequency / sampleRate;
        const float cosw0 = std::cos(w0);
        const float sinw0 = std::sin(w0);
        const float alpha = sinw0 / (2.0f * std::max(0.01f, q));
        const float A = std::pow(10.0f, gainDb / 40.0f);

        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (type) {
            case Type::Lowpass:
                b0 = (1.0f - cosw0) * 0.5f;
                b1 = 1.0f - cosw0;
                b2 = (1.0f - cosw0) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosw0;
                a2 = 1.0f - alpha;
                break;
            case Type::Highpass:
                b0 = (1.0f + cosw0) * 0.5f;
                b1 = -(1.0f + cosw0);
                b2 = (1.0f + cosw0) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosw0;
                a2 = 1.0f - alpha;
                break;
            case Type::Bandpass:
                b0 = alpha;
                b1 = 0.0f;
                b2 = -alpha;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosw0;
                a2 = 1.0f - alpha;
                break;
            case Type::HighShelf: {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
                b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
                a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
                break;
            }
            case Type::LowShelf: {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
                a2 = (A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
                break;
            }
        }

        m_b0 = b0 / a0;
        m_b1 = b1 / a0;
        m_b2 = b2 / a0;
        m_a1 = a1 / a0;
        m_a2 = a2 / a0;
    }

    void reset() noexcept {
        m_x1 = m_x2 = m_y1 = m_y2 = 0.0f;
    }

    inline float process(float x) noexcept {
        const float y = m_b0 * x + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = x;
        m_y2 = m_y1;
        m_y1 = y;
        return y;
    }

private:
    float m_b0 = 1.0f, m_b1 = 0.0f, m_b2 = 0.0f;
    float m_a1 = 0.0f, m_a2 = 0.0f;
    float m_x1 = 0.0f, m_x2 = 0.0f, m_y1 = 0.0f, m_y2 = 0.0f;
};

class FastRandom {
public:
    explicit FastRandom(uint32_t seed = 314159265) : m_state(seed == 0 ? 1 : seed) {}

    inline void setSeed(uint32_t seed) noexcept {
        m_state = (seed == 0 ? 1 : seed);
    }

    inline uint32_t nextInt() noexcept {
        uint32_t x = m_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_state = x;
        return x;
    }

    inline float nextFloat() noexcept {
        return static_cast<float>(nextInt()) * (1.0f / 4294967296.0f);
    }

    inline float nextSignedFloat() noexcept {
        return nextFloat() * 2.0f - 1.0f;
    }

private:
    uint32_t m_state;
};

} // namespace ff360
