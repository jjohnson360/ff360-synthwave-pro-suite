#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(FF360_DSP_EXPORTS)
    #define FF360_API __declspec(dllexport)
  #else
    #define FF360_API
  #endif
#else
  #define FF360_API __attribute__((visibility("default")))
#endif

// Opaque handles
typedef struct FF360_TapeEngine_T* FF360_TapeEngineHandle;
typedef struct FF360_ModulationEngine_T* FF360_ModulationEngineHandle;
typedef struct FF360_ReverbEngine_T* FF360_ReverbEngineHandle;
typedef struct FF360_MeteringBridge_T* FF360_MeteringBridgeHandle;

// TapeEngine C API
FF360_API FF360_TapeEngineHandle ff360_tape_create(void);
FF360_API void ff360_tape_destroy(FF360_TapeEngineHandle handle);
FF360_API void ff360_tape_prepare(FF360_TapeEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_tape_reset(FF360_TapeEngineHandle handle);
FF360_API void ff360_tape_set_parameter(FF360_TapeEngineHandle handle, uint32_t paramId, float normalizedValue);
FF360_API float ff360_tape_get_parameter(FF360_TapeEngineHandle handle, uint32_t paramId);
FF360_API void ff360_tape_set_degrade_macro(FF360_TapeEngineHandle handle, float degradeNormalized);
FF360_API void ff360_tape_process_stereo(FF360_TapeEngineHandle handle, float* left, float* right, size_t numSamples);

// ModulationEngine C API
FF360_API FF360_ModulationEngineHandle ff360_mod_create(void);
FF360_API void ff360_mod_destroy(FF360_ModulationEngineHandle handle);
FF360_API void ff360_mod_prepare(FF360_ModulationEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_mod_reset(FF360_ModulationEngineHandle handle);
FF360_API void ff360_mod_set_params(FF360_ModulationEngineHandle handle, float rateHz, float depth, float width, float detune, float feedback, float preDelayMs, float mix, float bassMonoHz, int quadMode, int vintageMode);
FF360_API void ff360_mod_process_stereo(FF360_ModulationEngineHandle handle, float* left, float* right, size_t numSamples);

// ReverbEngine C API
FF360_API FF360_ReverbEngineHandle ff360_reverb_create(void);
FF360_API void ff360_reverb_destroy(FF360_ReverbEngineHandle handle);
FF360_API void ff360_reverb_prepare(FF360_ReverbEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_reverb_reset(FF360_ReverbEngineHandle handle);
FF360_API void ff360_reverb_set_params(FF360_ReverbEngineHandle handle, int algorithm, float decayTime, float preDelayMs, int preDelaySync, float ducking, float width, float modulation, float lowDampHz, float highDampHz, int freeze, float mix);
FF360_API void ff360_reverb_process_stereo(FF360_ReverbEngineHandle handle, float* left, float* right, size_t numSamples);

// MeteringBridge C API
FF360_API FF360_MeteringBridgeHandle ff360_meter_create(void);
FF360_API void ff360_meter_destroy(FF360_MeteringBridgeHandle handle);
FF360_API void ff360_meter_prepare(FF360_MeteringBridgeHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_meter_reset(FF360_MeteringBridgeHandle handle);
FF360_API void ff360_meter_process_stereo(FF360_MeteringBridgeHandle handle, const float* left, const float* right, size_t numSamples);
FF360_API void ff360_meter_get_levels(FF360_MeteringBridgeHandle handle, float* outPeakL, float* outPeakR, float* outTruePeakL, float* outTruePeakR, float* outRmsL, float* outRmsR, float* outVuL, float* outVuR, float* outLra);

// StereoEngine C API
typedef struct FF360_StereoEngine_T* FF360_StereoEngineHandle;
FF360_API FF360_StereoEngineHandle ff360_stereo_create(void);
FF360_API void ff360_stereo_destroy(FF360_StereoEngineHandle handle);
FF360_API void ff360_stereo_prepare(FF360_StereoEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_stereo_reset(FF360_StereoEngineHandle handle);
FF360_API void ff360_stereo_set_params(FF360_StereoEngineHandle handle, float microDelayMs, float haasWidth, float detune, float msWidth, float freqWidth, float freqCrossoverHz, float rotationDeg, float bassMonoHz, float mix);
FF360_API void ff360_stereo_process(FF360_StereoEngineHandle handle, float* left, float* right, size_t numSamples);
FF360_API void ff360_stereo_get_metrics(FF360_StereoEngineHandle handle, float* outCorrelation, float* outBalance);

// GlitchEngine C API
typedef struct FF360_GlitchEngine_T* FF360_GlitchEngineHandle;
FF360_API FF360_GlitchEngineHandle ff360_glitch_create(void);
FF360_API void ff360_glitch_destroy(FF360_GlitchEngineHandle handle);
FF360_API void ff360_glitch_prepare(FF360_GlitchEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_glitch_reset(FF360_GlitchEngineHandle handle);
FF360_API void ff360_glitch_set_params(FF360_GlitchEngineHandle handle, int division, float bpm, float probability, int reverse, int freeze, float pitchShift, float bitcrush, float gate, float filterHz, float filterRes, float mix);
FF360_API void ff360_glitch_process(FF360_GlitchEngineHandle handle, float* left, float* right, size_t numSamples);

// GenerativeEngine C API
typedef struct FF360_GenerativeEngine_T* FF360_GenerativeEngineHandle;
FF360_API FF360_GenerativeEngineHandle ff360_gen_create(void);
FF360_API void ff360_gen_destroy(FF360_GenerativeEngineHandle handle);
FF360_API void ff360_gen_prepare(FF360_GenerativeEngineHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_gen_reset(FF360_GenerativeEngineHandle handle);
FF360_API void ff360_gen_select_generator(FF360_GenerativeEngineHandle handle, int generatorId);
FF360_API void ff360_gen_trigger(FF360_GenerativeEngineHandle handle, uint32_t seed);
FF360_API void ff360_gen_process(FF360_GenerativeEngineHandle handle, float* left, float* right, size_t numSamples);

// GranularTexture C API
typedef struct FF360_GranularTexture_T* FF360_GranularTextureHandle;
FF360_API FF360_GranularTextureHandle ff360_granular_create(void);
FF360_API void ff360_granular_destroy(FF360_GranularTextureHandle handle);
FF360_API void ff360_granular_prepare(FF360_GranularTextureHandle handle, double sampleRate, size_t maxBlockSize);
FF360_API void ff360_granular_reset(FF360_GranularTextureHandle handle);
FF360_API void ff360_granular_set_params(FF360_GranularTextureHandle handle, float grainSizeMs, float density, float pitchSpray, float spread, float evolve, uint32_t seed, float mix);
FF360_API void ff360_granular_process(FF360_GranularTextureHandle handle, float* left, float* right, size_t numSamples);

#ifdef __cplusplus
}
#endif
