#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

namespace ff360 {

struct ParameterDescriptor {
    std::string id;
    std::string name;
    std::string unit;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float smoothingTimeMs = 20.0f;
};

class SmoothedParameter {
public:
    SmoothedParameter() = default;
    
    void init(float initialValue, float smoothingTimeMs, double sampleRate) {
        m_targetValue = initialValue;
        m_currentValue = initialValue;
        setSmoothingTime(smoothingTimeMs, sampleRate);
    }

    void setSmoothingTime(float smoothingTimeMs, double sampleRate) {
        if (smoothingTimeMs <= 0.0f || sampleRate <= 0.0) {
            m_coeff = 1.0f;
        } else {
            const float samples = static_cast<float>(smoothingTimeMs * 0.001 * sampleRate);
            m_coeff = 1.0f - std::exp(-1.0f / std::max(1.0f, samples));
        }
    }

    void setTarget(float target) noexcept { m_targetValue = target; }
    void setCurrentAndTarget(float val) noexcept { m_currentValue = val; m_targetValue = val; }
    
    inline float getTarget() const noexcept { return m_targetValue; }
    inline float getCurrent() const noexcept { return m_currentValue; }

    inline float getNextValue() noexcept {
        m_currentValue += m_coeff * (m_targetValue - m_currentValue);
        return m_currentValue;
    }

    inline bool isSmoothing() const noexcept {
        return std::abs(m_targetValue - m_currentValue) > 1e-5f;
    }

private:
    float m_targetValue = 0.0f;
    float m_currentValue = 0.0f;
    float m_coeff = 1.0f;
};

class FF360_DSP_ParameterManager {
public:
    FF360_DSP_ParameterManager() = default;
    ~FF360_DSP_ParameterManager() = default;

    void registerParameter(const ParameterDescriptor& desc);
    void prepare(double sampleRate);

    void setValue(const std::string& id, float value);
    void setNormalizedValue(const std::string& id, float normalized);

    float getValue(const std::string& id) const;
    float getNormalizedValue(const std::string& id) const;
    float getSmoothedValue(const std::string& id);

    const std::vector<ParameterDescriptor>& getDescriptors() const noexcept { return m_descriptors; }
    const ParameterDescriptor* getDescriptor(const std::string& id) const;

    // Preset serialization to/from JSON string
    std::string exportToJson() const;
    bool importFromJson(const std::string& jsonString);

private:
    std::vector<ParameterDescriptor> m_descriptors;
    std::unordered_map<std::string, size_t> m_idToIndex;
    std::vector<SmoothedParameter> m_smoothedParams;
    double m_sampleRate = 44100.0;
};

} // namespace ff360
