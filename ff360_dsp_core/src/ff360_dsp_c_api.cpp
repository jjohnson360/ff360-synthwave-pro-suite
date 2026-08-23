#include "ff360/ff360_dsp_c_api.h"
#include "ff360/TapeEngine.h"
#include "ff360/ModulationEngine.h"
#include "ff360/ReverbEngine.h"
#include "ff360/MacroSystem.h"
#include "ff360/MeteringBridge.h"
#include "ff360/StereoEngine.h"
#include "ff360/GlitchEngine.h"
#include "ff360/TapeStopController.h"
#include "ff360/GenerativeEngine.h"
#include "ff360/RetroGenerators.h"
#include "ff360/GranularTexture.h"

struct FF360_TapeEngine_T {
    ff360::FF360_DSP_TapeEngine engine;
    ff360::FF360_DSP_MacroSystem degradeMacro = ff360::FF360_DSP_MacroSystem::createVhsDegradeMacro();
};

struct FF360_ModulationEngine_T {
    ff360::FF360_DSP_ModulationEngine engine;
};

struct FF360_ReverbEngine_T {
    ff360::FF360_DSP_ReverbEngine engine;
};

struct FF360_MeteringBridge_T {
    ff360::FF360_DSP_MeteringBridge bridge;
};

struct FF360_StereoEngine_T {
    ff360::FF360_DSP_StereoEngine engine;
};

struct FF360_GlitchEngine_T {
    ff360::FF360_DSP_GlitchEngine engine;
};

struct FF360_GenerativeEngine_T {
    ff360::FF360_DSP_GenerativeEngine engine;
    FF360_GenerativeEngine_T() {
        engine.registerGenerator(0, std::make_shared<ff360::NoiseSweepGenerator>());
        engine.registerGenerator(1, std::make_shared<ff360::PitchSweepGenerator>());
        engine.registerGenerator(2, std::make_shared<ff360::LaserGenerator>());
        engine.registerGenerator(3, std::make_shared<ff360::ReverseGenerator>());
        engine.registerGenerator(4, std::make_shared<ff360::ImpactGenerator>());
        engine.registerGenerator(5, std::make_shared<ff360::RiserGenerator>());
        engine.registerGenerator(6, std::make_shared<ff360::DownlifterGenerator>());
        engine.registerGenerator(7, std::make_shared<ff360::DigitalSweepGenerator>());
        engine.registerGenerator(8, std::make_shared<ff360::TapeSweepGenerator>());
    }
};

struct FF360_GranularTexture_T {
    ff360::FF360_DSP_GranularTexture engine;
};

// TapeEngine Implementation
FF360_TapeEngineHandle ff360_tape_create(void) {
    return new FF360_TapeEngine_T();
}

void ff360_tape_destroy(FF360_TapeEngineHandle handle) {
    delete handle;
}

void ff360_tape_prepare(FF360_TapeEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_tape_reset(FF360_TapeEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_tape_set_parameter(FF360_TapeEngineHandle handle, uint32_t paramId, float normalizedValue) {
    if (handle) handle->engine.setParameter(static_cast<ff360::TapeParamId>(paramId), normalizedValue);
}

float ff360_tape_get_parameter(FF360_TapeEngineHandle handle, uint32_t paramId) {
    if (handle) return handle->engine.getParameter(static_cast<ff360::TapeParamId>(paramId));
    return 0.0f;
}

void ff360_tape_set_degrade_macro(FF360_TapeEngineHandle handle, float degradeNormalized) {
    if (handle) {
        handle->degradeMacro.setMacroValue(degradeNormalized);
        handle->degradeMacro.apply([handle](uint32_t paramId, float val) {
            handle->engine.setParameter(static_cast<ff360::TapeParamId>(paramId), val);
        });
    }
}

void ff360_tape_process_stereo(FF360_TapeEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

// ModulationEngine Implementation
FF360_ModulationEngineHandle ff360_mod_create(void) {
    return new FF360_ModulationEngine_T();
}

void ff360_mod_destroy(FF360_ModulationEngineHandle handle) {
    delete handle;
}

void ff360_mod_prepare(FF360_ModulationEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_mod_reset(FF360_ModulationEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_mod_set_params(FF360_ModulationEngineHandle handle, float rateHz, float depth, float width, float detune, float feedback, float preDelayMs, float mix, float bassMonoHz, int quadMode, int vintageMode) {
    if (!handle) return;
    ff360::ModulationParameters p;
    p.rateHz = rateHz;
    p.depth = depth;
    p.width = width;
    p.detune = detune;
    p.feedback = feedback;
    p.preDelayMs = preDelayMs;
    p.mix = mix;
    p.bassMonoCrossoverHz = bassMonoHz;
    p.mode = (quadMode != 0) ? ff360::ChorusMode::Quad4Voice : ff360::ChorusMode::Stereo2Voice;
    p.character = (vintageMode != 0) ? ff360::ChorusCharacter::Vintage : ff360::ChorusCharacter::Modern;
    handle->engine.setParameters(p);
}

void ff360_mod_process_stereo(FF360_ModulationEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

// ReverbEngine Implementation
FF360_ReverbEngineHandle ff360_reverb_create(void) {
    return new FF360_ReverbEngine_T();
}

void ff360_reverb_destroy(FF360_ReverbEngineHandle handle) {
    delete handle;
}

void ff360_reverb_prepare(FF360_ReverbEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_reverb_reset(FF360_ReverbEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_reverb_set_params(FF360_ReverbEngineHandle handle, int algorithm, float decayTime, float preDelayMs, int preDelaySync, float ducking, float width, float modulation, float lowDampHz, float highDampHz, int freeze, float mix) {
    if (!handle) return;
    ff360::ReverbParameters p;
    p.algorithm = static_cast<ff360::ReverbAlgorithmType>(algorithm % static_cast<int>(ff360::ReverbAlgorithmType::Count));
    p.decayTime = decayTime;
    p.preDelayMs = preDelayMs;
    p.preDelaySync = (preDelaySync != 0);
    p.ducking = ducking;
    p.width = width;
    p.modulation = modulation;
    p.lowDampingHz = lowDampHz;
    p.highDampingHz = highDampHz;
    p.freeze = (freeze != 0);
    p.mix = mix;
    handle->engine.setParameters(p);
}

void ff360_reverb_process_stereo(FF360_ReverbEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

// MeteringBridge Implementation
FF360_MeteringBridgeHandle ff360_meter_create(void) {
    return new FF360_MeteringBridge_T();
}

void ff360_meter_destroy(FF360_MeteringBridgeHandle handle) {
    delete handle;
}

void ff360_meter_prepare(FF360_MeteringBridgeHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->bridge.prepare(sampleRate, maxBlockSize);
}

void ff360_meter_reset(FF360_MeteringBridgeHandle handle) {
    if (handle) handle->bridge.reset();
}

void ff360_meter_process_stereo(FF360_MeteringBridgeHandle handle, const float* left, const float* right, size_t numSamples) {
    if (handle) handle->bridge.processStereo(left, right, numSamples);
}

void ff360_meter_get_levels(FF360_MeteringBridgeHandle handle, float* outPeakL, float* outPeakR, float* outTruePeakL, float* outTruePeakR, float* outRmsL, float* outRmsR, float* outVuL, float* outVuR, float* outLra) {
    if (!handle) return;
    const auto lvl = handle->bridge.getLevels();
    if (outPeakL) *outPeakL = lvl.peakL;
    if (outPeakR) *outPeakR = lvl.peakR;
    if (outTruePeakL) *outTruePeakL = lvl.truePeakL;
    if (outTruePeakR) *outTruePeakR = lvl.truePeakR;
    if (outRmsL) *outRmsL = lvl.rmsL;
    if (outRmsR) *outRmsR = lvl.rmsR;
    if (outVuL) *outVuL = lvl.vuL;
    if (outVuR) *outVuR = lvl.vuR;
    if (outLra) *outLra = lvl.loudnessLra;
}

// StereoEngine Implementation
FF360_StereoEngineHandle ff360_stereo_create(void) {
    return new FF360_StereoEngine_T();
}

void ff360_stereo_destroy(FF360_StereoEngineHandle handle) {
    delete handle;
}

void ff360_stereo_prepare(FF360_StereoEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_stereo_reset(FF360_StereoEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_stereo_set_params(FF360_StereoEngineHandle handle, float microDelayMs, float haasWidth, float detune, float msWidth, float freqWidth, float freqCrossoverHz, float rotationDeg, float bassMonoHz, float mix) {
    if (!handle) return;
    ff360::StereoParameters p;
    p.microDelayMs = microDelayMs;
    p.haasWidth = haasWidth;
    p.stereoDetune = detune;
    p.msWidth = msWidth;
    p.freqWidth = freqWidth;
    p.freqWidthCrossoverHz = freqCrossoverHz;
    p.stereoRotationDeg = rotationDeg;
    p.bassMonoCutoffHz = bassMonoHz;
    p.mix = mix;
    handle->engine.setParameters(p);
}

void ff360_stereo_process(FF360_StereoEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

void ff360_stereo_get_metrics(FF360_StereoEngineHandle handle, float* outCorrelation, float* outBalance) {
    if (!handle) return;
    const auto m = handle->engine.getMetrics();
    if (outCorrelation) *outCorrelation = m.phaseCorrelation;
    if (outBalance) *outBalance = m.balance;
}

// GlitchEngine Implementation
FF360_GlitchEngineHandle ff360_glitch_create(void) {
    return new FF360_GlitchEngine_T();
}

void ff360_glitch_destroy(FF360_GlitchEngineHandle handle) {
    delete handle;
}

void ff360_glitch_prepare(FF360_GlitchEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_glitch_reset(FF360_GlitchEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_glitch_set_params(FF360_GlitchEngineHandle handle, int division, float bpm, float probability, int reverse, int freeze, float pitchShift, float bitcrush, float gate, float filterHz, float filterRes, float mix) {
    if (!handle) return;
    ff360::GlitchParameters p;
    p.division = static_cast<ff360::GlitchTimingDivision>(division % 4);
    p.hostBpm = bpm;
    p.probability = probability;
    p.reverse = (reverse != 0);
    p.freeze = (freeze != 0);
    p.pitchShiftSemitones = pitchShift;
    p.bitcrush = bitcrush;
    p.gate = gate;
    p.filterCutoffHz = filterHz;
    p.filterResonance = filterRes;
    p.mix = mix;
    handle->engine.setParameters(p);
}

void ff360_glitch_process(FF360_GlitchEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

// GenerativeEngine Implementation
FF360_GenerativeEngineHandle ff360_gen_create(void) {
    return new FF360_GenerativeEngine_T();
}

void ff360_gen_destroy(FF360_GenerativeEngineHandle handle) {
    delete handle;
}

void ff360_gen_prepare(FF360_GenerativeEngineHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_gen_reset(FF360_GenerativeEngineHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_gen_select_generator(FF360_GenerativeEngineHandle handle, int generatorId) {
    if (handle) handle->engine.selectGenerator(generatorId);
}

void ff360_gen_trigger(FF360_GenerativeEngineHandle handle, uint32_t seed) {
    if (handle) handle->engine.trigger(seed);
}

void ff360_gen_process(FF360_GenerativeEngineHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}

// GranularTexture Implementation
FF360_GranularTextureHandle ff360_granular_create(void) {
    return new FF360_GranularTexture_T();
}

void ff360_granular_destroy(FF360_GranularTextureHandle handle) {
    delete handle;
}

void ff360_granular_prepare(FF360_GranularTextureHandle handle, double sampleRate, size_t maxBlockSize) {
    if (handle) handle->engine.prepare(sampleRate, maxBlockSize);
}

void ff360_granular_reset(FF360_GranularTextureHandle handle) {
    if (handle) handle->engine.reset();
}

void ff360_granular_set_params(FF360_GranularTextureHandle handle, float grainSizeMs, float density, float pitchSpray, float spread, float evolve, uint32_t seed, float mix) {
    if (!handle) return;
    ff360::GranularParameters p;
    p.grainSizeMs = grainSizeMs;
    p.density = density;
    p.pitchSpraySemitones = pitchSpray;
    p.stereoSpread = spread;
    p.evolveRate = evolve;
    p.seed = seed;
    p.mix = mix;
    handle->engine.setParameters(p);
}

void ff360_granular_process(FF360_GranularTextureHandle handle, float* left, float* right, size_t numSamples) {
    if (handle) handle->engine.processStereo(left, right, numSamples);
}


