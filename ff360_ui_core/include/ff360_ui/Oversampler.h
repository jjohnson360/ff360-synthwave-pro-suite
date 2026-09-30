#pragma once

#if __has_include(<juce_dsp/juce_dsp.h>)
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>

namespace ff360_ui {

// Off / 2x / 4x oversampling around a plugin's nonlinear engine (saturation, bit reduction,
// generators), so harmonics above Nyquist are filtered out instead of folding back as aliasing.
//
// Polyphase IIR half-band filters (low latency, low CPU) with the latency rounded to whole
// samples, so the host can compensate it exactly and the dry path can be delayed to match.
//
// The engine must run at the oversampled rate, so changing the factor means re-preparing it:
// the processor does that on the message thread with processing suspended (see the plugins'
// applyOversampling()). process() itself never allocates.
class Oversampler {
public:
    static constexpr const char* paramId = "oversampling";
    static constexpr int kMaxOrder = 2; // 4x

    // Quality setting: never part of presets, undo or A/B (switching it re-prepares the engine)
    static void addParameter(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, int defaultOrder = 1) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ paramId, 1 }, "Oversampling", juce::StringArray{ "Off", "2x", "4x" }, defaultOrder));
    }

    static int readOrder(const juce::AudioProcessorValueTreeState& apvts) {
        auto* v = apvts.getRawParameterValue(paramId);
        return v != nullptr ? juce::jlimit(0, kMaxOrder, (int)v->load()) : 0;
    }

    void prepare(int numChannels, int maxBlockSize) {
        for (int order = 1; order <= kMaxOrder; ++order) {
            auto& os = m_stages[(size_t)order];
            os = std::make_unique<juce::dsp::Oversampling<float>>(
                (size_t)numChannels, (size_t)order, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                true, true);
            os->initProcessing((size_t)maxBlockSize);
        }
    }

    void setOrder(int order) { m_order = juce::jlimit(0, kMaxOrder, order); reset(); }
    int getOrder() const noexcept { return m_order; }
    int getFactor() const noexcept { return 1 << m_order; }

    // Whole samples at the host rate
    int getLatencySamples() const {
        return m_order == 0 || m_stages[(size_t)m_order] == nullptr
                   ? 0 : juce::roundToInt(m_stages[(size_t)m_order]->getLatencyInSamples());
    }

    void reset() {
        for (auto& os : m_stages)
            if (os != nullptr) os->reset();
    }

    // Runs fn(left, right, numSamples) on the first numChannels channels of buffer, at the
    // oversampled rate when oversampling is on. For one channel, left == right.
    template <typename Fn>
    void process(juce::AudioBuffer<float>& buffer, int numChannels, Fn&& fn) {
        const int n = buffer.getNumSamples();
        numChannels = juce::jlimit(1, 2, numChannels);

        if (m_order == 0 || m_stages[(size_t)m_order] == nullptr) {
            float* l = buffer.getWritePointer(0);
            fn(l, numChannels > 1 ? buffer.getWritePointer(1) : l, (size_t)n);
            return;
        }

        juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), (size_t)numChannels, (size_t)n);
        auto& os = *m_stages[(size_t)m_order];
        auto up = os.processSamplesUp(block);
        float* l = up.getChannelPointer(0);
        fn(l, numChannels > 1 ? up.getChannelPointer(1) : l, up.getNumSamples());
        os.processSamplesDown(block);
    }

private:
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, kMaxOrder + 1> m_stages; // [0] unused
    int m_order = 0;
};

} // namespace ff360_ui

#endif
