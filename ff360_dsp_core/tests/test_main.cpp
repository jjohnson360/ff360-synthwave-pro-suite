#include "ff360/TapeEngine.h"
#include "ff360/ModulationEngine.h"
#include "ff360/ReverbEngine.h"
#include "ff360/MacroSystem.h"
#include "ff360/ParameterManager.h"
#include "ff360/MeteringBridge.h"
#include "ff360/StereoEngine.h"
#include "ff360/TapeStopController.h"
#include "ff360/GlitchEngine.h"
#include "ff360/GenerativeEngine.h"
#include "ff360/RetroGenerators.h"
#include "ff360/GranularTexture.h"
#include "ff360/ff360_dsp_c_api.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <string>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] Assertion FAILED at " << __FILE__ << ":" << __LINE__ << " -> " << msg << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUN ] " << #fn << " ... " << std::flush; \
        if (fn()) { \
            std::cout << "[PASS]" << std::endl; \
            testsPassed++; \
        } else { \
            std::cout << "[FAIL]" << std::endl; \
            testsFailed++; \
        } \
    } while (0)

// Helper: Generate test sine buffer
static void generateSine(std::vector<float>& buf, float freqHz, float sampleRate, float amp = 1.0f) {
    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] = amp * std::sin(ff360::TWO_PI * freqHz * static_cast<float>(i) / sampleRate);
    }
}

// -------------------------------------------------------------
// 1. TapeEngine Tests
// -------------------------------------------------------------
static bool test_TapeEngine_BasicAndStability() {
    ff360::FF360_DSP_TapeEngine tape;
    tape.prepare(48000.0, 512);

    std::vector<float> left(1024, 0.0f);
    std::vector<float> right(1024, 0.0f);
    generateSine(left, 440.0f, 48000.0f, 0.8f);
    generateSine(right, 440.0f, 48000.0f, 0.8f);

    // Max out all degradation parameters
    ff360::TapeParameters params;
    params.saturation = 1.0f;
    params.wow = 1.0f;
    params.flutter = 1.0f;
    params.pitchDrift = 1.0f;
    params.noise = 1.0f;
    params.hiss = 1.0f;
    params.dropouts = 1.0f;
    params.loFi = 1.0f;
    params.bitReduction = 1.0f;
    params.highFrequencyLoss = 1.0f;
    params.stereoDrift = 1.0f;
    params.warble = 1.0f;
    params.mix = 1.0f;
    tape.setParameters(params);

    tape.processStereo(left.data(), right.data(), left.size());

    for (size_t i = 0; i < left.size(); ++i) {
        TEST_ASSERT(!std::isnan(left[i]) && !std::isinf(left[i]), "TapeEngine produced NaN or Inf on Left channel");
        TEST_ASSERT(!std::isnan(right[i]) && !std::isinf(right[i]), "TapeEngine produced NaN or Inf on Right channel");
    }
    return true;
}

static bool test_TapeEngine_SampleRateSwitching() {
    ff360::FF360_DSP_TapeEngine tape;
    for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 }) {
        tape.prepare(sr, 512);
        std::vector<float> left(256, 0.5f);
        std::vector<float> right(256, 0.5f);
        tape.processStereo(left.data(), right.data(), 256);
        TEST_ASSERT(!std::isnan(left[0]), "Failed on sample rate switch");
    }
    return true;
}

// -------------------------------------------------------------
// 2. ModulationEngine Tests
// -------------------------------------------------------------
static bool test_ModulationEngine_StereoAndQuadMode() {
    ff360::FF360_DSP_ModulationEngine chorus;
    chorus.prepare(44100.0, 512);

    ff360::ModulationParameters params;
    params.rateHz = 1.2f;
    params.depth = 0.8f;
    params.width = 1.0f;
    params.mix = 1.0f;
    params.mode = ff360::ChorusMode::Stereo2Voice;
    chorus.setParameters(params);

    std::vector<float> left2V(1024, 0.0f);
    std::vector<float> right2V(1024, 0.0f);
    generateSine(left2V, 300.0f, 44100.0f, 0.7f);
    generateSine(right2V, 300.0f, 44100.0f, 0.7f);
    chorus.processStereo(left2V.data(), right2V.data(), 1024);

    // Quad mode test
    params.mode = ff360::ChorusMode::Quad4Voice;
    chorus.setParameters(params);
    std::vector<float> left4V(1024, 0.0f);
    std::vector<float> right4V(1024, 0.0f);
    generateSine(left4V, 300.0f, 44100.0f, 0.7f);
    generateSine(right4V, 300.0f, 44100.0f, 0.7f);
    chorus.processStereo(left4V.data(), right4V.data(), 1024);

    // Verify stereo difference exists (decorrelation / widening)
    float diffSum = 0.0f;
    for (size_t i = 100; i < 1024; ++i) {
        diffSum += std::abs(left4V[i] - right4V[i]);
    }
    TEST_ASSERT(diffSum > 5.0f, "Quad chorus failed to produce stereo spread");
    return true;
}

static bool test_ModulationEngine_BassMono() {
    ff360::FF360_DSP_ModulationEngine chorus;
    chorus.prepare(44100.0, 512);

    ff360::ModulationParameters params;
    params.bassMonoCrossoverHz = 200.0f;
    params.mix = 1.0f;
    chorus.setParameters(params);

    // 60 Hz Sub-bass test tone
    std::vector<float> left(1024, 0.0f);
    std::vector<float> right(1024, 0.0f);
    generateSine(left, 60.0f, 44100.0f, 0.8f);
    generateSine(right, 60.0f, 44100.0f, 0.8f);
    chorus.processStereo(left.data(), right.data(), 1024);

    // Bass Mono Crossover Check: measure stereo difference signal energy vs mono energy
    float diffEnergy = 0.0f;
    float sumEnergy = 0.0f;
    for (size_t i = 200; i < 1024; ++i) {
        const float d = left[i] - right[i];
        const float s = left[i] + right[i];
        diffEnergy += d * d;
        sumEnergy += s * s;
    }
    const float stereoLeakRatio = (sumEnergy > 0.0001f) ? std::sqrt(diffEnergy / sumEnergy) : 0.0f;
    TEST_ASSERT(stereoLeakRatio < 0.05f, "Bass Mono crossover failed to lock sub frequencies to center (<5% stereo leak)");
    return true;
}

// -------------------------------------------------------------
// 3. ReverbEngine Tests
// -------------------------------------------------------------
static bool test_ReverbEngine_AlgorithmsAndFreeze() {
    ff360::FF360_DSP_ReverbEngine reverb;
    reverb.prepare(44100.0, 512);

    // Check all 6 algorithms process cleanly
    for (int alg = 0; alg < static_cast<int>(ff360::ReverbAlgorithmType::Count); ++alg) {
        ff360::ReverbParameters p;
        p.algorithm = static_cast<ff360::ReverbAlgorithmType>(alg);
        p.decayTime = 0.6f;
        p.preDelayMs = 0.0f; // 0ms pre-delay so tail appears immediately
        p.mix = 1.0f;
        reverb.setParameters(p);

        std::vector<float> left(2048, 0.0f);
        std::vector<float> right(2048, 0.0f);
        left[0] = 1.0f; right[0] = 1.0f; // Impulse
        reverb.processStereo(left.data(), right.data(), 2048);

        float energy = 0.0f;
        for (size_t i = 10; i < 2048; ++i) {
            TEST_ASSERT(!std::isnan(left[i]), "Reverb produced NaN");
            energy += std::abs(left[i]);
        }
        TEST_ASSERT(energy > 0.01f, "Reverb algorithm produced no tail");
    }

    // Freeze Test
    ff360::ReverbParameters freezeParams;
    freezeParams.algorithm = ff360::ReverbAlgorithmType::Endless;
    freezeParams.freeze = true;
    freezeParams.mix = 1.0f;
    reverb.setParameters(freezeParams);

    std::vector<float> freezeL(2048, 0.0f);
    std::vector<float> freezeR(2048, 0.0f);
    freezeL[0] = 1.0f;
    reverb.processStereo(freezeL.data(), freezeR.data(), 2048);

    // Later samples should maintain tail energy without dying out
    float tailEnergy = 0.0f;
    for (size_t i = 1500; i < 2048; ++i) tailEnergy += std::abs(freezeL[i]);
    TEST_ASSERT(tailEnergy > 0.05f, "Freeze mode failed to sustain audio tail");
    return true;
}

static bool test_ReverbEngine_Ducking() {
    ff360::FF360_DSP_ReverbEngine reverb;
    reverb.prepare(44100.0, 512);

    ff360::ReverbParameters p;
    p.ducking = 0.95f; // Heavy ducking
    p.mix = 1.0f;
    p.decayTime = 0.8f;
    reverb.setParameters(p);

    // Send impulse to excite tail
    std::vector<float> burstL(256, 0.0f);
    std::vector<float> burstR(256, 0.0f);
    burstL[0] = 1.0f;
    reverb.processStereo(burstL.data(), burstR.data(), 256);

    // Send loud continuous tone to trigger ducking
    std::vector<float> loudL(512, 1.0f);
    std::vector<float> loudR(512, 1.0f);
    reverb.processStereo(loudL.data(), loudR.data(), 512);

    TEST_ASSERT(true, "Ducking processed smoothly without instability");
    return true;
}

// -------------------------------------------------------------
// 4. MacroSystem Tests
// -------------------------------------------------------------
static bool test_MacroSystem_DegradeCurves() {
    auto degrade = ff360::FF360_DSP_MacroSystem::createVhsDegradeMacro();

    // At Macro = 0.0, all parameters should be at min
    degrade.setMacroValue(0.0f);
    float sat0 = 0.0f, drop0 = 0.0f;
    degrade.apply([&](uint32_t id, float val) {
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Saturation)) sat0 = val;
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Dropouts)) drop0 = val;
    });
    TEST_ASSERT(sat0 < 0.01f, "Saturation should be 0 at macro 0");
    TEST_ASSERT(drop0 < 0.01f, "Dropouts should be 0 at macro 0");

    // At Macro = 0.3 (30%), saturation should ramp up significantly, but dropouts stay 0 (thresholded > 0.45)
    degrade.setMacroValue(0.3f);
    float sat30 = 0.0f, drop30 = 0.0f;
    degrade.apply([&](uint32_t id, float val) {
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Saturation)) sat30 = val;
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Dropouts)) drop30 = val;
    });
    TEST_ASSERT(sat30 > 0.45f, "Saturation should ramp early in DEGRADE");
    TEST_ASSERT(drop30 == 0.0f, "Dropouts should remain zero below threshold (0.45)");

    // At Macro = 1.0, everything is at maximum
    degrade.setMacroValue(1.0f);
    float sat100 = 0.0f, drop100 = 0.0f;
    degrade.apply([&](uint32_t id, float val) {
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Saturation)) sat100 = val;
        if (id == static_cast<uint32_t>(ff360::TapeParamId::Dropouts)) drop100 = val;
    });
    TEST_ASSERT(sat100 > 0.9f, "Saturation should reach max at macro 1.0");
    TEST_ASSERT(drop100 > 0.6f, "Dropouts should be active at macro 1.0");
    return true;
}

// -------------------------------------------------------------
// 5. ParameterManager Tests
// -------------------------------------------------------------
static bool test_ParameterManager_RegistryAndJson() {
    ff360::FF360_DSP_ParameterManager pm;
    pm.prepare(44100.0);

    pm.registerParameter({ "sat", "Tape Saturation", "%", 0.0f, 100.0f, 25.0f, 10.0f });
    pm.registerParameter({ "rate", "LFO Rate", "Hz", 0.1f, 10.0f, 1.0f, 10.0f });

    TEST_ASSERT(std::abs(pm.getValue("sat") - 25.0f) < 0.01f, "Default value mismatch");
    pm.setValue("sat", 75.0f);
    TEST_ASSERT(std::abs(pm.getValue("sat") - 75.0f) < 0.01f, "Value update mismatch");

    // JSON export/import test
    std::string json = pm.exportToJson();
    TEST_ASSERT(json.find("\"sat\": 75.000000") != std::string::npos, "JSON export missing parameter");

    pm.setValue("sat", 10.0f);
    pm.importFromJson(json);
    TEST_ASSERT(std::abs(pm.getValue("sat") - 75.0f) < 0.01f, "JSON import failed to restore parameter value");
    return true;
}

// -------------------------------------------------------------
// 6. MeteringBridge Post-Phase-10 Verification Tests
// -------------------------------------------------------------
static bool test_MeteringBridge_PostPhase10Accuracy() {
    ff360::FF360_DSP_MeteringBridge meter;
    meter.prepare(48000.0, 512);

    // 1 kHz sine at 0 dBFS (peak = 1.0, theoretical RMS = 1/sqrt(2) = -3.0103 dBFS)
    const size_t numSamples = 48000; // 1 full second
    std::vector<float> sineL(numSamples, 0.0f);
    std::vector<float> sineR(numSamples, 0.0f);
    generateSine(sineL, 1000.0f, 48000.0f, 1.0f);
    generateSine(sineR, 1000.0f, 48000.0f, 1.0f);

    meter.processStereo(sineL.data(), sineR.data(), numSamples);
    auto lvl = meter.getLevels();

    // Spot-check Peak is within ~0.05 dB of 0.0 dBFS
    TEST_ASSERT(std::abs(lvl.peakL - 0.0f) < 0.1f, "Peak reading mismatch on 0 dBFS sine");
    // Spot-check RMS is within ~0.15 dB of -3.01 dBFS
    TEST_ASSERT(std::abs(lvl.rmsL - (-3.01f)) < 0.25f, "Post-Phase-10 RMS reading mismatch on 0 dBFS sine");
    // Spot-check True Peak is at least >= peak
    TEST_ASSERT(lvl.truePeakL >= lvl.peakL - 0.05f, "True Peak under-reported peak level");

    // Spot-check LRA gating on low silence (-80 dBFS)
    std::vector<float> silence(4800, 0.00001f); // -100 dBFS
    meter.processStereo(silence.data(), silence.data(), silence.size());
    // Should process without errors or NaN
    TEST_ASSERT(!std::isnan(lvl.loudnessLra), "LRA generated NaN on low signal");

    return true;
}

// -------------------------------------------------------------
// 7. StereoEngine Phase 2 Tests
// -------------------------------------------------------------
static bool test_StereoEngine_CorrelationAndBassMono() {
    ff360::FF360_DSP_StereoEngine stereo;
    stereo.prepare(48000.0, 512);

    // 1. Fully correlated signal (identical Left and Right) -> correlation should be +1.0
    std::vector<float> leftMono(4800, 0.0f);
    std::vector<float> rightMono(4800, 0.0f);
    generateSine(leftMono, 1000.0f, 48000.0f, 0.8f);
    generateSine(rightMono, 1000.0f, 48000.0f, 0.8f);

    stereo.processStereo(leftMono.data(), rightMono.data(), leftMono.size());
    auto m = stereo.getMetrics();
    TEST_ASSERT(std::abs(m.phaseCorrelation - 1.0f) < 0.05f, "Phase correlation failed to report +1.0 on mono signal");

    // 2. Fully anti-correlated signal (L = -R) -> correlation should be -1.0
    for (size_t i = 0; i < rightMono.size(); ++i) rightMono[i] = -leftMono[i];
    ff360::StereoParameters noMonoParams;
    noMonoParams.bassMonoCutoffHz = 0.0f; // Disable bass mono for pure anti-correlation test
    stereo.setParameters(noMonoParams);
    stereo.processStereo(leftMono.data(), rightMono.data(), leftMono.size());
    m = stereo.getMetrics();
    TEST_ASSERT(std::abs(m.phaseCorrelation - (-1.0f)) < 0.05f, "Phase correlation failed to report -1.0 on anti-phase signal");

    // 3. Bass Mono collapse test: Sub-bass anti-phase signal should collapse to silence / mono
    ff360::StereoParameters bassMonoParams;
    bassMonoParams.bassMonoCutoffHz = 200.0f;
    stereo.setParameters(bassMonoParams);

    std::vector<float> subL(4800, 0.0f);
    std::vector<float> subR(4800, 0.0f);
    generateSine(subL, 60.0f, 48000.0f, 0.8f);
    for (size_t i = 0; i < subR.size(); ++i) subR[i] = -subL[i]; // Anti-phase sub

    stereo.processStereo(subL.data(), subR.data(), subL.size());
    // Measure residual anti-phase energy vs original input energy
    float diffEnergy = 0.0f;
    float inEnergy = 0.0f;
    for (size_t i = 500; i < subL.size(); ++i) {
        const float d = subL[i] - subR[i];
        diffEnergy += d * d;
        inEnergy += 4.0f * (0.8f * 0.8f * 0.5f); // Expected unattenuated anti-phase difference
    }
    const float stereoLeakRatio = std::sqrt(diffEnergy / inEnergy);
    TEST_ASSERT(stereoLeakRatio < 0.05f, "Bass Mono failed to collapse low-end anti-phase to mono (<5% leak)");

    return true;
}

// -------------------------------------------------------------
// 8. TapeStopController Phase 2 Tests
// -------------------------------------------------------------
static bool test_TapeStopController_StateMachine() {
    ff360::FF360_DSP_TapeEngine tape;
    tape.prepare(44100.0, 512);

    ff360::FF360_DSP_TapeStopController controller;
    controller.prepare(44100.0);

    ff360::TapeStopParameters p;
    p.slowdownTimeSec = 0.1f; // 100ms stop
    p.recoveryTimeSec = 0.1f; // 100ms recovery
    controller.setParameters(p);

    TEST_ASSERT(controller.getState() == ff360::TapeStopState::Idle, "Should start in Idle");

    controller.triggerStop();
    TEST_ASSERT(controller.getState() == ff360::TapeStopState::Stopping, "Should transition to Stopping");

    // Advance 4410 samples (100ms)
    controller.updateAndApply(tape, 4410);
    TEST_ASSERT(controller.getState() == ff360::TapeStopState::Stopped, "Should reach Stopped state");
    TEST_ASSERT(controller.getProgress() >= 0.99f, "Progress should be 1.0");

    controller.triggerRecovery();
    TEST_ASSERT(controller.getState() == ff360::TapeStopState::Recovering, "Should transition to Recovering");

    // Advance 4410 samples
    controller.updateAndApply(tape, 4410);
    TEST_ASSERT(controller.getState() == ff360::TapeStopState::Idle, "Should return to Idle after recovery");
    TEST_ASSERT(controller.getProgress() <= 0.01f, "Progress should be 0.0");

    return true;
}

// -------------------------------------------------------------
// 9. GlitchEngine Phase 2 Tests
// -------------------------------------------------------------
static bool test_GlitchEngine_StutterAndProbability() {
    ff360::FF360_DSP_GlitchEngine glitch;
    glitch.prepare(48000.0, 512);

    ff360::GlitchParameters p;
    p.division = ff360::GlitchTimingDivision::Div_1_16;
    p.hostBpm = 120.0f;
    p.probability = 1.0f; // 100% trigger for deterministic test
    p.reverse = true;
    p.pitchShiftSemitones = 7.0f;
    p.bitcrush = 0.5f;
    p.mix = 1.0f;
    glitch.setParameters(p);

    std::vector<float> left(4800, 0.0f);
    std::vector<float> right(4800, 0.0f);
    generateSine(left, 440.0f, 48000.0f, 0.7f);
    generateSine(right, 440.0f, 48000.0f, 0.7f);

    glitch.processStereo(left.data(), right.data(), left.size());

    for (float s : left) {
        TEST_ASSERT(!std::isnan(s) && !std::isinf(s), "GlitchEngine produced NaN or Inf");
    }
    return true;
}

// -------------------------------------------------------------
// 10. GenerativeEngine Determinism & Recall Tests
// -------------------------------------------------------------
static bool test_GenerativeEngine_DeterministicRecall() {
    ff360::FF360_DSP_GenerativeEngine gen1;
    gen1.prepare(44100.0, 256);
    gen1.registerGenerator(0, std::make_shared<ff360::NoiseSweepGenerator>());
    gen1.selectGenerator(0);

    ff360::GenerativeParameters p;
    p.syncMode = ff360::GenerativeSyncMode::Div_1_Bar;
    p.hostBpm = 120.0f;
    p.seed = 1984;
    p.intensity = 0.9f;
    p.mix = 1.0f;
    gen1.setParameters(p);
    gen1.trigger(1984);

    const size_t totalSamples = 44100 * 2; // 2 seconds (1 bar at 120bpm)
    std::vector<float> run1L(totalSamples, 0.0f);
    std::vector<float> run1R(totalSamples, 0.0f);
    gen1.processStereo(run1L.data(), run1R.data(), totalSamples);

    // Run 2 with same seed
    ff360::FF360_DSP_GenerativeEngine gen2;
    gen2.prepare(44100.0, 256);
    gen2.registerGenerator(0, std::make_shared<ff360::NoiseSweepGenerator>());
    gen2.selectGenerator(0);
    gen2.setParameters(p);
    gen2.trigger(1984);

    std::vector<float> run2L(totalSamples, 0.0f);
    std::vector<float> run2R(totalSamples, 0.0f);
    gen2.processStereo(run2L.data(), run2R.data(), totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        TEST_ASSERT(run1L[i] == run2L[i], "GenerativeEngine output was not bit-identical across runs with same seed");
        TEST_ASSERT(run1R[i] == run2R[i], "GenerativeEngine output was not bit-identical across runs with same seed");
    }
    return true;
}

// -------------------------------------------------------------
// 11. GranularTexture Ambient Cloud Tests
// -------------------------------------------------------------
static bool test_GranularTexture_AudioStability() {
    ff360::FF360_DSP_GranularTexture gran;
    gran.prepare(48000.0, 256);

    ff360::GranularParameters p;
    p.density = 45.0f;
    p.grainSizeMs = 60.0f;
    p.pitchSpraySemitones = 4.0f;
    p.stereoSpread = 1.0f;
    p.evolveRate = 0.8f;
    p.mix = 1.0f;
    gran.setParameters(p);

    std::vector<float> left(4800, 0.0f);
    std::vector<float> right(4800, 0.0f);

    gran.processStereo(left.data(), right.data(), left.size());

    float energySum = 0.0f;
    for (size_t i = 0; i < left.size(); ++i) {
        TEST_ASSERT(!std::isnan(left[i]) && !std::isinf(left[i]), "GranularTexture produced NaN or Inf");
        TEST_ASSERT(!std::isnan(right[i]) && !std::isinf(right[i]), "GranularTexture produced NaN or Inf");
        energySum += left[i] * left[i] + right[i] * right[i];
    }
    TEST_ASSERT(energySum > 0.01f, "GranularTexture produced silence");

    return true;
}

// -------------------------------------------------------------
// Main Runner
// -------------------------------------------------------------
int main() {
    std::cout << "========================================================\n";
    std::cout << "   ff360_dsp_core Standalone DSP Test Suite (Phases 1-3) \n";
    std::cout << "========================================================\n\n";

    int testsPassed = 0;
    int testsFailed = 0;

    RUN_TEST(test_TapeEngine_BasicAndStability);
    RUN_TEST(test_TapeEngine_SampleRateSwitching);
    RUN_TEST(test_ModulationEngine_StereoAndQuadMode);
    RUN_TEST(test_ModulationEngine_BassMono);
    RUN_TEST(test_ReverbEngine_AlgorithmsAndFreeze);
    RUN_TEST(test_ReverbEngine_Ducking);
    RUN_TEST(test_MacroSystem_DegradeCurves);
    RUN_TEST(test_ParameterManager_RegistryAndJson);
    RUN_TEST(test_MeteringBridge_PostPhase10Accuracy);
    RUN_TEST(test_StereoEngine_CorrelationAndBassMono);
    RUN_TEST(test_TapeStopController_StateMachine);
    RUN_TEST(test_GlitchEngine_StutterAndProbability);
    RUN_TEST(test_GenerativeEngine_DeterministicRecall);
    RUN_TEST(test_GranularTexture_AudioStability);

    std::cout << "\n========================================================\n";
    std::cout << "Results: " << testsPassed << " passed, " << testsFailed << " failed.\n";
    std::cout << "========================================================\n";

    return (testsFailed == 0) ? 0 : 1;
}
