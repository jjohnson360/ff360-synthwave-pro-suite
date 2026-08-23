#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <functional>

namespace ff360 {

enum class CurveType {
    Linear = 0,
    Exponential,  // Slow start, rapid ramp at end (t^2 or t^3)
    Logarithmic,  // Rapid start, tapers off (sqrt(t) or log)
    SCurve,       // Smooth ease-in ease-out
    Thresholded   // Zero below threshold, ramps up above threshold
};

struct MacroTargetMapping {
    uint32_t parameterId = 0;
    CurveType curve = CurveType::Linear;
    float minOutput = 0.0f;
    float maxOutput = 1.0f;
    float curveExponent = 2.0f; // For Exp/Log/SCurve
    float threshold = 0.0f;     // For Thresholded curve
};

class FF360_DSP_MacroSystem {
public:
    FF360_DSP_MacroSystem() = default;
    ~FF360_DSP_MacroSystem() = default;

    void clearMappings() { m_mappings.clear(); }
    void addMapping(const MacroTargetMapping& mapping) { m_mappings.push_back(mapping); }

    void setMacroValue(float normalizedValue) noexcept {
        m_macroValue = clamp(normalizedValue, 0.0f, 1.0f);
    }
    float getMacroValue() const noexcept { return m_macroValue; }

    // Evaluates the mapped value for a given mapping at current macro value
    float evaluateMapping(const MacroTargetMapping& mapping) const noexcept;

    // Apply all mapped values to a target callback: void(uint32_t paramId, float mappedVal)
    void apply(const std::function<void(uint32_t, float)>& setter) const {
        for (const auto& mapping : m_mappings) {
            setter(mapping.parameterId, evaluateMapping(mapping));
        }
    }

    // Factory helper for VHS DEGRADE configuration
    static FF360_DSP_MacroSystem createVhsDegradeMacro();

private:
    float m_macroValue = 0.0f;
    std::vector<MacroTargetMapping> m_mappings;
};

} // namespace ff360
