#include "ff360/TapeStopController.h"
#include <algorithm>

namespace ff360 {

void FF360_DSP_TapeStopController::prepare(double sampleRate) {
    m_sampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    reset();
}

void FF360_DSP_TapeStopController::reset() {
    m_state = TapeStopState::Idle;
    m_progress = 0.0f;
}

void FF360_DSP_TapeStopController::triggerStop() {
    if (m_state == TapeStopState::Idle || m_state == TapeStopState::Recovering) {
        m_state = TapeStopState::Stopping;
    }
}

void FF360_DSP_TapeStopController::triggerRecovery() {
    if (m_state == TapeStopState::Stopping || m_state == TapeStopState::Stopped) {
        m_state = TapeStopState::Recovering;
    }
}

void FF360_DSP_TapeStopController::toggleStop() {
    if (m_state == TapeStopState::Idle || m_state == TapeStopState::Recovering) {
        triggerStop();
    } else {
        triggerRecovery();
    }
}

void FF360_DSP_TapeStopController::updateAndApply(FF360_DSP_TapeEngine& tapeEngine, size_t numSamples) {
    const float dt = static_cast<float>(numSamples) / static_cast<float>(m_sampleRate);

    // Update state progression
    switch (m_state) {
        case TapeStopState::Idle:
            m_progress = 0.0f;
            break;

        case TapeStopState::Stopping: {
            const float stopRate = (m_params.slowdownTimeSec > 0.001f) ? (1.0f / m_params.slowdownTimeSec) : 100.0f;
            m_progress += dt * stopRate;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_state = TapeStopState::Stopped;
            }
            break;
        }

        case TapeStopState::Stopped:
            m_progress = 1.0f;
            break;

        case TapeStopState::Recovering: {
            const float recRate = (m_params.recoveryTimeSec > 0.001f) ? (1.0f / m_params.recoveryTimeSec) : 100.0f;
            m_progress -= dt * recRate;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_state = TapeStopState::Idle;
            }
            break;
        }
    }

    // Evaluate pitch slowdown curve based on selected profile
    float curvedPitch = m_progress;
    float hfLossMod = 0.0f;
    float lofiMod = 0.0f;
    float wowFlutterBoost = 0.0f;

    switch (m_params.profile) {
        case TapeStopProfile::VinylStop:
            // S-Curve pitch drop with turntable motor drag
            curvedPitch = m_progress * m_progress * (3.0f - 2.0f * m_progress);
            wowFlutterBoost = std::sin(m_progress * PI) * 0.8f;
            hfLossMod = m_progress * m_params.filterMovement * 0.6f;
            break;

        case TapeStopProfile::TapeStop:
            // Exponential inertia drop with dynamic HF loss
            curvedPitch = std::pow(m_progress, lerp(1.2f, 3.5f, m_params.pitchCurve));
            wowFlutterBoost = std::sin(m_progress * PI) * 0.95f;
            hfLossMod = m_progress * m_params.filterMovement;
            break;

        case TapeStopProfile::DigitalStop:
            // Stepped quantization and rapid cut
            curvedPitch = std::pow(m_progress, 1.8f);
            lofiMod = m_progress * 0.85f;
            hfLossMod = m_progress * 0.4f;
            break;
    }

    // Apply modulation into TapeEngine parameters
    tapeEngine.setParameter(TapeParamId::PitchDrift, curvedPitch);
    tapeEngine.setParameter(TapeParamId::Wow, wowFlutterBoost * 0.7f);
    tapeEngine.setParameter(TapeParamId::Flutter, wowFlutterBoost * 0.5f);
    tapeEngine.setParameter(TapeParamId::HighFrequencyLoss, hfLossMod);
    if (lofiMod > 0.01f) {
        tapeEngine.setParameter(TapeParamId::LoFi, lofiMod);
        tapeEngine.setParameter(TapeParamId::BitReduction, lofiMod * 0.5f);
    }
}

} // namespace ff360
