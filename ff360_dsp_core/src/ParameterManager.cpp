#include "ff360/ParameterManager.h"
#include <sstream>
#include <iomanip>

namespace ff360 {

void FF360_DSP_ParameterManager::registerParameter(const ParameterDescriptor& desc) {
    auto it = m_idToIndex.find(desc.id);
    if (it != m_idToIndex.end()) {
        m_descriptors[it->second] = desc;
        return;
    }
    const size_t index = m_descriptors.size();
    m_descriptors.push_back(desc);
    m_idToIndex[desc.id] = index;

    SmoothedParameter sp;
    sp.init(desc.defaultValue, desc.smoothingTimeMs, m_sampleRate);
    m_smoothedParams.push_back(sp);
}

void FF360_DSP_ParameterManager::prepare(double sampleRate) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    for (size_t i = 0; i < m_descriptors.size(); ++i) {
        m_smoothedParams[i].setSmoothingTime(m_descriptors[i].smoothingTimeMs, m_sampleRate);
    }
}

void FF360_DSP_ParameterManager::setValue(const std::string& id, float value) {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        const size_t idx = it->second;
        const auto& desc = m_descriptors[idx];
        const float clamped = clamp(value, desc.minValue, desc.maxValue);
        m_smoothedParams[idx].setTarget(clamped);
    }
}

void FF360_DSP_ParameterManager::setNormalizedValue(const std::string& id, float normalized) {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        const size_t idx = it->second;
        const auto& desc = m_descriptors[idx];
        const float value = lerp(desc.minValue, desc.maxValue, clamp(normalized, 0.0f, 1.0f));
        m_smoothedParams[idx].setTarget(value);
    }
}

float FF360_DSP_ParameterManager::getValue(const std::string& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return m_smoothedParams[it->second].getTarget();
    }
    return 0.0f;
}

float FF360_DSP_ParameterManager::getNormalizedValue(const std::string& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        const size_t idx = it->second;
        const auto& desc = m_descriptors[idx];
        if (std::abs(desc.maxValue - desc.minValue) < 1e-6f) return 0.0f;
        return (m_smoothedParams[idx].getTarget() - desc.minValue) / (desc.maxValue - desc.minValue);
    }
    return 0.0f;
}

float FF360_DSP_ParameterManager::getSmoothedValue(const std::string& id) {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return m_smoothedParams[it->second].getNextValue();
    }
    return 0.0f;
}

const ParameterDescriptor* FF360_DSP_ParameterManager::getDescriptor(const std::string& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return &m_descriptors[it->second];
    }
    return nullptr;
}

std::string FF360_DSP_ParameterManager::exportToJson() const {
    std::ostringstream ss;
    ss << "{\n  \"version\": 1,\n  \"parameters\": {\n";
    for (size_t i = 0; i < m_descriptors.size(); ++i) {
        const auto& d = m_descriptors[i];
        const float val = m_smoothedParams[i].getTarget();
        ss << "    \"" << d.id << "\": " << std::fixed << std::setprecision(6) << val;
        if (i + 1 < m_descriptors.size()) ss << ",";
        ss << "\n";
    }
    ss << "  }\n}";
    return ss.str();
}

bool FF360_DSP_ParameterManager::importFromJson(const std::string& jsonString) {
    // Robust, lightweight standard JSON key-value parser
    for (size_t i = 0; i < m_descriptors.size(); ++i) {
        const auto& d = m_descriptors[i];
        const std::string key = "\"" + d.id + "\"";
        size_t pos = jsonString.find(key);
        if (pos != std::string::npos) {
            pos = jsonString.find(':', pos);
            if (pos != std::string::npos) {
                size_t start = pos + 1;
                while (start < jsonString.size() && (jsonString[start] == ' ' || jsonString[start] == '\t')) {
                    ++start;
                }
                size_t end = start;
                while (end < jsonString.size() && (jsonString[end] == '-' || jsonString[end] == '.' || (jsonString[end] >= '0' && jsonString[end] <= '9'))) {
                    ++end;
                }
                if (end > start) {
                    try {
                        const float val = std::stof(jsonString.substr(start, end - start));
                        setValue(d.id, val);
                    } catch (...) {}
                }
            }
        }
    }
    return true;
}

} // namespace ff360
