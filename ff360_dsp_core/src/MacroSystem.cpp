#include "ff360/MacroSystem.h"
#include "ff360/TapeEngine.h"
#include <cmath>

namespace ff360 {

float FF360_DSP_MacroSystem::evaluateMapping(const MacroTargetMapping& mapping) const noexcept {
    const float t = m_macroValue;
    float curvedT = t;

    switch (mapping.curve) {
        case CurveType::Linear:
            curvedT = t;
            break;
        case CurveType::Exponential:
            curvedT = std::pow(t, std::max(1.0f, mapping.curveExponent));
            break;
        case CurveType::Logarithmic:
            curvedT = std::pow(t, 1.0f / std::max(1.0f, mapping.curveExponent));
            break;
        case CurveType::SCurve:
            // Smoothstep formula: 3t^2 - 2t^3
            curvedT = t * t * (3.0f - 2.0f * t);
            break;
        case CurveType::Thresholded:
            if (t <= mapping.threshold) {
                curvedT = 0.0f;
            } else {
                const float range = 1.0f - mapping.threshold;
                curvedT = (range > 0.0001f) ? (t - mapping.threshold) / range : 1.0f;
                curvedT = std::pow(curvedT, std::max(1.0f, mapping.curveExponent));
            }
            break;
    }

    return lerp(mapping.minOutput, mapping.maxOutput, curvedT);
}

FF360_DSP_MacroSystem FF360_DSP_MacroSystem::createVhsDegradeMacro() {
    FF360_DSP_MacroSystem ms;

    // Saturation and Noise ramp early (Logarithmic)
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Saturation), CurveType::Logarithmic, 0.0f, 0.95f, 2.2f, 0.0f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Noise), CurveType::Logarithmic, 0.0f, 0.70f, 2.0f, 0.0f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Hiss), CurveType::Logarithmic, 0.0f, 0.65f, 1.8f, 0.0f });

    // High frequency loss ramps steadily (Linear / SCurve)
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::HighFrequencyLoss), CurveType::SCurve, 0.0f, 0.85f, 2.0f, 0.0f });

    // Wow and Flutter ramp progressively (Linear)
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Wow), CurveType::Linear, 0.0f, 0.80f, 1.0f, 0.0f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Flutter), CurveType::Linear, 0.0f, 0.75f, 1.0f, 0.0f });

    // Pitch drift, Stereo drift, Warble ramp later in the sweep (Exponential)
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::PitchDrift), CurveType::Exponential, 0.0f, 0.85f, 2.5f, 0.0f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::StereoDrift), CurveType::Exponential, 0.0f, 0.80f, 2.0f, 0.0f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Warble), CurveType::Exponential, 0.0f, 0.70f, 2.2f, 0.0f });

    // Dropouts and Lo-Fi only engage heavily when DEGRADE is pushed hard (Thresholded above 45%)
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::Dropouts), CurveType::Thresholded, 0.0f, 0.65f, 1.8f, 0.45f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::LoFi), CurveType::Thresholded, 0.0f, 0.55f, 2.0f, 0.50f });
    ms.addMapping({ static_cast<uint32_t>(TapeParamId::BitReduction), CurveType::Thresholded, 0.0f, 0.50f, 2.0f, 0.60f });

    return ms;
}

} // namespace ff360
