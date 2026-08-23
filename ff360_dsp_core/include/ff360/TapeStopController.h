#pragma once

#include "Common.h"
#include "TapeEngine.h"
#include <cmath>

namespace ff360 {

enum class TapeStopState {
    Idle = 0,
    Stopping,
    Stopped,
    Recovering
};

enum class TapeStopProfile {
    VinylStop = 0,   // S-curve pitch dip with turntable friction
    TapeStop,        // Exponential inertia slowdown + HF filter loss
    DigitalStop      // Stepped bit/lo-fi decimation slowdown + hard cut
};

struct TapeStopParameters {
    float slowdownTimeSec = 0.5f;   // 0.05 to 5.0 seconds
    float pitchCurve = 0.5f;        // 0.0 (concave) to 1.0 (convex/S-curve)
    float filterMovement = 0.7f;    // Dynamic lowpass filter sweep depth
    float recoveryTimeSec = 0.3f;   // Recovery spin-up time
    bool reverseRecovery = false;   // Spin back up in reverse phase
    TapeStopProfile profile = TapeStopProfile::TapeStop;
};

class FF360_DSP_TapeStopController {
public:
    FF360_DSP_TapeStopController() = default;
    ~FF360_DSP_TapeStopController() = default;

    void prepare(double sampleRate);
    void reset();

    void triggerStop();
    void triggerRecovery();
    void toggleStop();

    void setParameters(const TapeStopParameters& params) noexcept { m_params = params; }
    const TapeStopParameters& getParameters() const noexcept { return m_params; }

    TapeStopState getState() const noexcept { return m_state; }
    float getProgress() const noexcept { return m_progress; }

    // Advances the state machine by numSamples and applies modulation into TapeEngine
    void updateAndApply(FF360_DSP_TapeEngine& tapeEngine, size_t numSamples);

private:
    double m_sampleRate = 44100.0;
    TapeStopParameters m_params;
    TapeStopState m_state = TapeStopState::Idle;
    float m_progress = 0.0f; // 0.0 (normal running) to 1.0 (fully stopped)
};

} // namespace ff360
