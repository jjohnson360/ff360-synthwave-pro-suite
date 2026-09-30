// Workflow tests for the shared preset / undo / A-B bar (ff360_ui::WorkflowBar) in all 8 plugins.
//
// Builds every plugin's real processor and editor into one app (createPluginFilter is renamed per
// plugin in CMake) and checks: preset load + modified flag, undo/redo, A/B, user presets, the
// clipboard, and session save/restore. Each editor is also rendered to a PNG for a visual check.
//
//   cmake -B build -DFF360_BUILD_WORKFLOW_TESTS=ON
//   cmake --build build --config Release --target ff360_workflow_tests
//   build/ff360_workflow_tests_artefacts/Release/ff360_workflow_tests.exe [snapshot folder]

#include "vhs/PluginProcessor.h"
#include "vhs/PluginEditor.h"
#include "neon_chorus/PluginProcessor.h"
#include "neon_chorus/PluginEditor.h"
#include "midnight_reverb/PluginProcessor.h"
#include "midnight_reverb/PluginEditor.h"
#include "neon_width/PluginProcessor.h"
#include "neon_width/PluginEditor.h"
#include "neon_tape_stop/PluginProcessor.h"
#include "neon_tape_stop/PluginEditor.h"
#include "cyberpunk_glitch/PluginProcessor.h"
#include "cyberpunk_glitch/PluginEditor.h"
#include "retrofx/PluginProcessor.h"
#include "retrofx/PluginEditor.h"
#include "nightdrive/PluginProcessor.h"
#include "nightdrive/PluginEditor.h"

#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>

namespace {

int failures = 0;
int checks = 0;
juce::String currentPlugin;

void check(bool ok, const juce::String& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cout << "  FAIL [" << currentPlugin << "] " << what << "\n";
    }
}

bool near(float a, float b, float tol = 1.0e-3f) { return std::abs(a - b) <= tol; }

// A UI edit: a gesture around the change, like a knob drag
void edit(juce::RangedAudioParameter& p, float normalised) {
    p.beginChangeGesture();
    p.setValueNotifyingHost(normalised);
    p.endChangeGesture();
}


// Every visible control in a component tree, depth first
void collectControls(juce::Component& root, std::vector<juce::Component*>& out) {
    for (auto* c : root.getChildren()) {
        if (!c->isVisible()) continue;
        if (dynamic_cast<juce::Slider*>(c) || dynamic_cast<juce::Button*>(c) || dynamic_cast<juce::ComboBox*>(c)
            || dynamic_cast<ff360_ui::FF360_HeroKnob*>(c))
            out.push_back(c);
        collectControls(*c, out);
    }
}

juce::String tooltipOf(juce::Component* c) {
    if (auto* t = dynamic_cast<juce::TooltipClient*>(c)) return t->getTooltip();
    return {};
}

// Text of every visible label in the editor
juce::StringArray labelTexts(juce::Component& root) {
    juce::StringArray texts;
    for (auto* c : root.getChildren()) {
        if (auto* l = dynamic_cast<juce::Label*>(c)) texts.add(l->getText());
        texts.addArray(labelTexts(*c));
    }
    return texts;
}

template <typename Proc>
void testPlugin(const juce::String& name, const std::vector<ff360::Preset>& factory, const juce::File& snapshotDir) {
    currentPlugin = name;
    std::cout << name << "\n";

    Proc proc;
    proc.setRateAndBufferSizeDetails(48000.0, 512); // as a host does before preparing
    proc.prepareToPlay(48000.0, 512);
    auto& apvts = proc.getApvts();
    auto& history = proc.getHistory();
    auto& presets = proc.getPresetManager();
    auto* mix = apvts.getParameter("mix");
    check(mix != nullptr, "has a mix parameter");
    if (mix == nullptr) return;

    // ---- Fresh instance ----
    check(presets.getCurrentName() == "Default", "starts on Default");
    check(!presets.isModified(), "starts unmodified");
    check(presets.getNumPresets() >= (int)factory.size() + 1, "lists Default + factory presets");
    check(proc.getNumPrograms() == 1, "host sees a single program");

    // ---- Every factory preset loads exactly its JSON values ----
    for (int f = 0; f < (int)factory.size(); ++f) {
        presets.loadPreset(f + 1);
        check(presets.getCurrentName() == juce::String(factory[(size_t)f].name), "loads factory preset " + juce::String(f));
        check(!presets.isModified(), "unmodified right after loading " + presets.getCurrentName());

        auto* values = juce::JSON::parse(juce::String(factory[(size_t)f].jsonContent)).getProperty("parameters", {}).getDynamicObject();
        check(values != nullptr, "preset JSON parses: " + presets.getCurrentName());
        if (values == nullptr) continue;
        for (const auto& prop : values->getProperties()) {
            auto* p = apvts.getParameter(prop.name.toString());
            check(p != nullptr, "preset key is a parameter: " + prop.name.toString());
            if (p != nullptr)
                check(near(p->getValue(), p->convertTo0to1((float)prop.value)),
                      presets.getCurrentName() + " sets " + prop.name.toString());
        }
    }

    // Default puts every preset parameter back to its default
    presets.loadPreset(0);
    check(presets.getCurrentName() == "Default", "Default loads");
    check(near(mix->getValue(), mix->getDefaultValue()), "Default restores the mix default");

    // ---- Modified flag and undo/redo ----
    presets.loadPreset(1);
    const float presetMix = mix->getValue();
    const float editedMix = presetMix > 0.5f ? 0.25f : 0.75f;
    edit(*mix, editedMix);
    check(presets.isModified(), "an edit marks the preset modified");
    history.commit(); // normally done by the history's timer after the gesture ends

    check(history.canUndo(), "an edit can be undone");
    history.undo();
    check(near(mix->getValue(), presetMix), "undo restores the value");
    check(!presets.isModified(), "undo back to the preset clears the modified dot");
    check(history.canRedo(), "an undo can be redone");
    history.redo();
    check(near(mix->getValue(), editedMix), "redo re-applies the value");

    // Loading a preset is itself one undo step
    presets.loadPreset(2);
    history.undo();
    check(near(mix->getValue(), editedMix), "undo reverts a preset load");

    // ---- A/B ----
    edit(*mix, 0.2f);
    check(history.getActiveSlot() == 0, "starts on slot A");
    history.switchSlot();
    check(history.getActiveSlot() == 1, "switches to B");
    check(near(mix->getValue(), 0.2f), "B starts as a copy of A");
    edit(*mix, 0.8f);
    history.switchSlot();
    check(near(mix->getValue(), 0.2f), "back on A: A's value");
    history.switchSlot();
    check(near(mix->getValue(), 0.8f), "back on B: B's value");
    check(!history.canUndo(), "switching slots starts a fresh undo history");

    // ---- Session save / restore (host project reload) ----
    const auto savedName = presets.getCurrentName();
    const bool savedModified = presets.isModified();
    juce::MemoryBlock state;
    proc.getStateInformation(state);
    {
        Proc restored;
        restored.setStateInformation(state.getData(), (int)state.getSize());
        auto* rmix = restored.getApvts().getParameter("mix");
        check(near(rmix->getValue(), 0.8f), "session restores parameter values");
        check(restored.getPresetManager().getCurrentName() == savedName, "session restores the preset name");
        check(restored.getPresetManager().isModified() == savedModified, "session restores the modified flag");
        check(restored.getHistory().getActiveSlot() == 1, "session restores the active A/B slot");
        restored.getHistory().switchSlot();
        check(near(rmix->getValue(), 0.2f), "session restores the inactive A/B slot");
    }

    // Sessions saved before this feature (bare parameter state) still load
    {
        auto bare = apvts.copyState().createXml();
        for (auto* attr : { "presetName", "presetIsUser", "presetModified", "abActive", "abInactive" })
            bare->removeAttribute(attr);
        juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary(*bare, old);
        Proc restored;
        restored.setStateInformation(old.getData(), (int)old.getSize());
        check(near(restored.getApvts().getParameter("mix")->getValue(), 0.8f), "pre-preset-bar sessions load");
        check(restored.getPresetManager().isModified(), "pre-preset-bar sessions show as modified");
    }

    // ---- User presets ----
    const juce::String userName = "zz ff360 workflow test";
    edit(*mix, 0.33f);
    presets.saveUserPreset(userName);
    check(presets.isCurrentUserPreset() && presets.getCurrentName() == userName, "saved preset becomes current");
    check(!presets.isModified(), "saved preset is unmodified");
    const int userIndex = presets.getCurrentIndex();
    check(userIndex > (int)factory.size(), "user preset is listed after the factory presets");
    presets.loadPreset(0);
    presets.loadPreset(userIndex);
    check(near(mix->getValue(), 0.33f), "user preset loads its values");
    presets.deleteUserPreset(presets.getCurrentIndex());
    check(!presets.userPresetExists(userName), "user preset deletes");
    check(!presets.isCurrentUserPreset(), "deleted preset is no longer current");

    // ---- Clipboard ----
    // The Windows clipboard can be briefly locked by other apps (clipboard history, cloud sync),
    // so give the round trip a few tries before calling it a failure
    bool pasted = false;
    for (int attempt = 0; attempt < 5 && !pasted; ++attempt) {
        edit(*mix, 0.6f);
        presets.copyToClipboard();
        edit(*mix, 0.1f);
        pasted = presets.pasteFromClipboard();
        if (!pasted) juce::Thread::sleep(100);
    }
    check(pasted, "pastes its own preset");
    check(near(mix->getValue(), 0.6f), "paste applies the copied values");


    // ---- Output stage in the plugin: host bypass, bypass null, finite output ----
    check(apvts.getParameter("outGain") != nullptr, "has an output trim");
    auto* bypass = apvts.getParameter("bypass");
    check(bypass != nullptr && proc.getBypassParameter() == bypass, "host bypass maps to the bypass parameter");
    if (bypass != nullptr) {
        juce::AudioBuffer<float> buf(std::max(proc.getTotalNumInputChannels(), proc.getTotalNumOutputChannels()), 512);
        juce::MidiBuffer midi;
        juce::Random rng(42);
        auto fillNoise = [&] {
            buf.clear();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i)
                    buf.setSample(ch, i, (rng.nextFloat() * 2.0f - 1.0f) * 0.25f);
        };

        edit(*bypass, 1.0f);
        presets.loadPreset(1);
        check(bypass->getValue() > 0.5f, "loading a preset doesn't change bypass");

        // Bypassed output is the input delayed by the reported latency (oversampling), bit for bit
        const int latency = proc.getLatencySamples();
        std::vector<float> inHist[2], outHist[2];
        for (int block = 0; block < 6; ++block) {
            fillNoise();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i) inHist[ch].push_back(buf.getSample(ch, i));
            proc.processBlock(buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i) outHist[ch].push_back(buf.getSample(ch, i));
        }
        float maxDiff = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (size_t i = 1024 + (size_t)latency; i < outHist[ch].size(); ++i) // after the 10 ms fade
                maxDiff = std::max(maxDiff, std::abs(outHist[ch][i] - inHist[ch][i - (size_t)latency]));
        check(maxDiff == 0.0f, "bypassed output is the input, bit for bit, " + juce::String(latency)
              + " samples late (max diff " + juce::String(maxDiff) + ")");

        edit(*bypass, 0.0f);
        bool finite = true;
        for (int block = 0; block < 100; ++block) {
            fillNoise();
            proc.processBlock(buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i)
                    finite = finite && std::isfinite(buf.getSample(ch, i));
        }
        check(finite, "processing stays finite");
    }

    // ---- Editor ----
    std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
    check(editor->getWidth() == 350 && editor->getHeight() == 712, "editor is 350 x 712");

    // Every control explains itself on hover
    {
        std::vector<juce::Component*> controls;
        collectControls(*editor, controls);
        check(controls.size() >= 8, "editor has controls to check");
        for (auto* c : controls) {
            auto* b = dynamic_cast<juce::Button*>(c);
            check(tooltipOf(c).isNotEmpty(), "control has a tooltip: " + c->getName()
                  + (b != nullptr ? " '" + b->getButtonText() + "'" : juce::String()));
        }
        // Sliders show their value with its unit while dragging
        int withPopup = 0;
        for (auto* c : controls)
            if (auto* sl = dynamic_cast<juce::Slider*>(c); sl != nullptr && (bool)sl->getProperties()["ff360ShowsValue"]) ++withPopup;
        int sliders = 0;
        for (auto* c : controls) if (dynamic_cast<juce::Slider*>(c)) ++sliders;
        check(withPopup == sliders, "every slider shows its value (popup or readout) (" + juce::String(withPopup) + "/" + juce::String(sliders) + ")");
    }
    auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
    if (snapshotDir != juce::File()) {
        auto file = snapshotDir.getChildFile(name.removeCharacters(" ") + ".png");
        file.deleteFile();
        juce::FileOutputStream out(file);
        juce::PNGImageFormat().writeImageToStream(snapshot, out);
    }
    editor.reset();
    proc.releaseResources();
}


// ff360::FF360_DSP_OutputStage on its own, with a plain gain standing in for the effect
void testOutputStage() {
    currentPlugin = "OutputStage";
    std::cout << "OutputStage\n";
    using Stage = ff360::FF360_DSP_OutputStage;
    constexpr double sr = 48000.0;
    constexpr size_t n = 512;
    juce::Random rng(7);
    std::vector<float> inL(n), inR(n), L(n), R(n);

    // Runs `blocks` blocks of noise at `level` through an "effect" of `effectGain`; returns out/in RMS in dB
    auto run = [&](Stage& st, float effectGain, float level, const Stage::Settings& s, int blocks) {
        double inSum = 0.0, outSum = 0.0;
        for (int b = 0; b < blocks; ++b) {
            for (size_t i = 0; i < n; ++i) {
                inL[i] = (rng.nextFloat() * 2.0f - 1.0f) * level;
                inR[i] = (rng.nextFloat() * 2.0f - 1.0f) * level;
            }
            st.captureDry(inL.data(), inR.data(), n);
            for (size_t i = 0; i < n; ++i) { L[i] = inL[i] * effectGain; R[i] = inR[i] * effectGain; }
            st.process(L.data(), R.data(), n, s);
            if (b == blocks - 1)
                for (size_t i = 0; i < n; ++i) {
                    inSum += inL[i] * inL[i] + inR[i] * inR[i];
                    outSum += L[i] * L[i] + R[i] * R[i];
                }
        }
        return inSum > 0.0 ? 10.0 * std::log10(outSum / inSum) : 0.0;
    };
    const int secs = (int)(sr / n); // blocks per second

    {
        Stage st; st.prepare(sr, n);
        Stage::Settings s; s.outputGainDb = 6.0f;
        check(std::abs(run(st, 1.0f, 0.3f, s, 3) - 6.0) < 0.05, "trim +6 dB gives +6 dB");
        s.outputGainDb = -12.0f;
        check(std::abs(run(st, 1.0f, 0.3f, s, 3) + 12.0) < 0.05, "trim -12 dB gives -12 dB");
    }
    {
        Stage st; st.prepare(sr, n);
        Stage::Settings s; s.autoGain = true;
        const double matched = run(st, 0.5f, 0.3f, s, 10 * secs); // effect is 6 dB quieter
        check(std::abs(matched) < 0.3, "auto gain matches the input level (off by " + juce::String(matched, 2) + " dB)");
        check(std::abs(st.getAppliedAutoGainDb() - 6.02f) < 0.3f, "auto gain applies about +6 dB");
        run(st, 0.5f, 0.0f, s, 3 * secs); // input goes silent
        check(std::abs(st.getAppliedAutoGainDb() - 6.02f) < 0.3f, "auto gain holds through silence");
        s.outputGainDb = -3.0f;
        check(std::abs(run(st, 0.5f, 0.3f, s, 3) + 3.0) < 0.3, "the trim still works on top of auto gain");
        s.autoGain = false; s.outputGainDb = 0.0f;
        check(std::abs(run(st, 0.5f, 0.3f, s, secs) + 6.02) < 0.1, "switching auto gain off removes it");
    }
    {
        Stage st; st.prepare(sr, n);
        Stage::Settings s; s.autoGain = true;
        run(st, 0.01f, 0.3f, s, 20 * secs); // effect is 40 dB quieter
        check(std::abs(st.getAppliedAutoGainDb() - 12.0f) < 0.1f, "auto gain is limited to +12 dB");
    }
    {
        Stage st; st.prepare(sr, n);
        Stage::Settings s; s.bypass = true;
        run(st, 0.5f, 0.3f, s, 1);
        const int fade = (int)std::ceil(0.010 * sr);
        bool dryAfterFade = true;
        for (int i = fade; i < (int)n; ++i)
            dryAfterFade = dryAfterFade && L[(size_t)i] == inL[(size_t)i] && R[(size_t)i] == inR[(size_t)i];
        check(dryAfterFade, "bypass reaches the dry signal within 10 ms");
        check(L[0] != inL[0], "bypass fades rather than jumping");
    }
    {
        // Mono: the host gives the same pointer for both channels; the gain must apply once
        Stage st; st.prepare(sr, n);
        Stage::Settings s; s.outputGainDb = 6.0f;
        for (int b = 0; b < 3; ++b) {
            for (size_t i = 0; i < n; ++i) inL[i] = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            st.captureDry(inL.data(), inL.data(), n);
            L = inL;
            st.process(L.data(), L.data(), n, s);
        }
        check(std::abs(L[100] / inL[100] - 1.9953f) < 1.0e-3f, "mono gets the gain once, not twice");
    }
}

// Energy (dB) in the FFT bins around each frequency, for a mono signal at 48 kHz
double bandEnergyDb(const std::vector<float>& x, std::initializer_list<double> freqs) {
    constexpr int order = 14, size = 1 << order; // 16384 points, ~2.9 Hz bins
    juce::dsp::FFT fft(order);
    std::vector<float> data(2 * size, 0.0f);
    for (int i = 0; i < size; ++i) {
        const float w = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(size - 1)); // Hann
        data[(size_t)i] = x[x.size() - size + (size_t)i] * w;
    }
    fft.performFrequencyOnlyForwardTransform(data.data());
    double energy = 1.0e-30;
    for (double f : freqs) {
        const int bin = (int)std::round(f / 48000.0 * size);
        for (int b = bin - 3; b <= bin + 3; ++b) energy += (double)data[(size_t)b] * data[(size_t)b];
    }
    return 10.0 * std::log10(energy);
}

// Sets a parameter's plain value (no gesture)
void setPlain(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float plain) {
    if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(plain));
}

// Oversampling in one plugin: latency per factor, and the setting survives a session reload
template <typename Proc>
void testOversamplingLatency(const juce::String& name) {
    currentPlugin = name;
    Proc proc;
    proc.setRateAndBufferSizeDetails(48000.0, 512);
    auto& apvts = proc.getApvts();
    check(apvts.getParameter("oversampling") != nullptr, "has an Oversampling setting");
    int latencies[3] = {};
    for (int order = 0; order <= 2; ++order) {
        setPlain(apvts, "oversampling", (float)order);
        proc.prepareToPlay(48000.0, 512); // applies it, as the async switch does
        latencies[order] = proc.getLatencySamples();
    }
    check(latencies[0] == 0, "Off adds no latency");
    check(latencies[1] > 0 && latencies[1] < 16, "2x reports a small latency (" + juce::String(latencies[1]) + ")");
    check(latencies[2] > 0 && latencies[2] < 16, "4x reports a small latency (" + juce::String(latencies[2]) + ")");
    check(proc.getPresetManager().getNumPresets() > 1 && [&] {
        setPlain(apvts, "oversampling", 2.0f);
        proc.getPresetManager().loadPreset(1);
        return apvts.getParameter("oversampling")->getValue() == 1.0f;
    }(), "loading a preset leaves Oversampling alone");
}

// VHS saturation on a 7 kHz sine: its 5th and 7th harmonics (35 / 49 kHz) fold back to
// 13 kHz and 1 kHz without oversampling. Returns the energy at those alias frequencies.
double vhsAliasEnergy(int order) {
    VHSPluginProcessor proc;
    auto& apvts = proc.getApvts();
    for (auto* id : { "wow", "flutter", "drift", "noise", "hiss", "dropouts", "lofi", "bitcrush",
                      "hfloss", "stereodrift", "warble", "degrade" })
        setPlain(apvts, id, 0.0f);
    setPlain(apvts, "sat", 100.0f);
    setPlain(apvts, "inGain", 12.0f);
    setPlain(apvts, "mix", 100.0f);
    setPlain(apvts, "oversampling", (float)order);
    proc.setRateAndBufferSizeDetails(48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    std::vector<float> out;
    double phase = 0.0;
    for (int block = 0; block < 64; ++block) {
        for (int i = 0; i < 512; ++i) {
            const float v = 0.5f * (float)std::sin(phase);
            phase += juce::MathConstants<double>::twoPi * 7000.0 / 48000.0;
            buf.setSample(0, i, v);
            buf.setSample(1, i, v);
        }
        proc.processBlock(buf, midi);
        out.insert(out.end(), buf.getReadPointer(0), buf.getReadPointer(0) + 512);
    }
    return bandEnergyDb(out, { 13000.0, 1000.0 }) - bandEnergyDb(out, { 7000.0 }); // relative to the fundamental
}

void testOversampling() {
    std::cout << "Oversampling\n";
    testOversamplingLatency<VHSPluginProcessor>("VHS");
    testOversamplingLatency<NeonTapeStopProcessor>("Neon Tape Stop");
    testOversamplingLatency<CyberpunkGlitchProcessor>("Cyberpunk Glitch");
    testOversamplingLatency<RetroFXProcessor>("RetroFX");

    currentPlugin = "VHS aliasing";
    const double off = vhsAliasEnergy(0), x2 = vhsAliasEnergy(1), x4 = vhsAliasEnergy(2);
    std::cout << "  aliasing vs fundamental: off " << juce::String(off, 1) << " dB, 2x " << juce::String(x2, 1)
              << " dB, 4x " << juce::String(x4, 1) << " dB\n";
    check(off > -80.0, "the test signal does alias without oversampling (" + juce::String(off, 1) + " dB)");
    check(x2 < off - 12.0, "2x cuts aliasing by more than 12 dB");
    check(x4 < off - 20.0, "4x cuts aliasing by more than 20 dB");
}


// Delta listen in one plugin: at 0% mix the effect adds nothing, so delta must be silence
// (this also proves the dry part cancels through the oversampler); with the effect in, it isn't
template <typename Proc>
void testDeltaCancels(const juce::String& name) {
    currentPlugin = name;
    Proc proc;
    auto& apvts = proc.getApvts();
    check(apvts.getParameter("delta") != nullptr, "has Delta listen");
    setPlain(apvts, "mix", 0.0f);
    setPlain(apvts, "delta", 1.0f);
    proc.setRateAndBufferSizeDetails(48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buf(std::max(2, proc.getTotalNumInputChannels()), 512);
    juce::MidiBuffer midi;
    juce::Random rng(3);
    auto runPeak = [&](int blocks) {
        float peak = 0.0f;
        for (int b = 0; b < blocks; ++b) {
            buf.clear();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i) buf.setSample(ch, i, (rng.nextFloat() * 2.0f - 1.0f) * 0.5f);
            proc.processBlock(buf, midi);
            if (b >= blocks / 2) peak = std::max(peak, buf.getMagnitude(0, 0, 512));
        }
        return peak;
    };
    const float silent = runPeak(20);
    check(silent < 1.0e-5f, "delta at 0% mix is silent (peak " + juce::String(juce::Decibels::gainToDecibels(silent), 1) + " dB)");

    setPlain(apvts, "mix", 100.0f);
    const float audible = runPeak(40);
    check(audible > 1.0e-3f, "delta at 100% mix carries the effect");

    proc.getPresetManager().loadPreset(1);
    check(apvts.getParameter("delta")->getValue() > 0.5f, "loading a preset leaves Delta alone");
}

// Reverb delta is the wet signal alone: an impulse's dry click is gone, its tail is not
void testReverbDeltaIsWetOnly() {
    currentPlugin = "Midnight Reverb delta";
    auto run = [](bool delta, float& early, float& late) {
        MidnightReverbProcessor proc;
        auto& apvts = proc.getApvts();
        setPlain(apvts, "mix", 35.0f);
        setPlain(apvts, "predelay", 30.0f);
        setPlain(apvts, "delta", delta ? 1.0f : 0.0f);
        proc.setRateAndBufferSizeDetails(48000.0, 512);
        proc.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> buf(std::max(2, proc.getTotalNumInputChannels()), 512);
        juce::MidiBuffer midi;
        std::vector<float> out;
        for (int b = 0; b < 40; ++b) {
            buf.clear();
            if (b == 4) { buf.setSample(0, 0, 1.0f); buf.setSample(1, 0, 1.0f); } // after the 10 ms fade
            proc.processBlock(buf, midi);
            out.insert(out.end(), buf.getReadPointer(0), buf.getReadPointer(0) + 512);
        }
        const size_t hit = 4 * 512;
        early = 0.0f; late = 0.0f;
        for (size_t i = hit; i < hit + 48 * 10; ++i) early = std::max(early, std::abs(out[i]));   // first 10 ms
        for (size_t i = hit + 48 * 40; i < out.size(); ++i) late = std::max(late, std::abs(out[i])); // after 40 ms
    };
    float earlyOff, lateOff, earlyOn, lateOn;
    run(false, earlyOff, lateOff);
    run(true, earlyOn, lateOn);
    check(earlyOff > 0.5f, "without delta the dry click comes through (" + juce::String(earlyOff, 3) + ")");
    check(earlyOn < 1.0e-4f, "with delta the dry click is gone (" + juce::String(earlyOn, 6) + ")");
    check(lateOn > 1.0e-4f && std::abs(lateOn - lateOff) < 0.25f * lateOff + 1.0e-6f,
          "with delta the reverb tail is unchanged (" + juce::String(lateOn, 5) + " vs " + juce::String(lateOff, 5) + ")");
}

void testDelta() {
    std::cout << "Delta\n";
    testDeltaCancels<VHSPluginProcessor>("VHS");
    testDeltaCancels<CyberpunkGlitchProcessor>("Cyberpunk Glitch");
    testDeltaCancels<MidnightReverbProcessor>("Midnight Reverb");
    testDeltaCancels<NeonChorusProcessor>("Neon Chorus");
    testReverbDeltaIsWetOnly();
}


// Resizable editors and the brand font
void testScalingAndFonts(const juce::File& snapshotDir) {
    std::cout << "Scaling and fonts\n";
    currentPlugin = "scaling";
    {
        NightDriveProcessor proc;
        proc.setEditorScale(1.5f);
        std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
        check(editor->getWidth() == 525 && editor->getHeight() == 1068,
              "editor opens at its saved size (" + juce::String(editor->getWidth()) + " x " + juce::String(editor->getHeight()) + ")");
        check(editor->isResizable(), "editor is resizable");
        auto* c = editor->getConstrainer();
        check(c != nullptr && std::abs(c->getFixedAspectRatio() - 350.0 / 712.0) < 1.0e-6, "aspect ratio is locked");
        check(c != nullptr && c->getMinimumWidth() == 262 && c->getMaximumWidth() == 700, "75% to 200% (" + juce::String(c ? c->getMinimumWidth() : -1) + " to " + juce::String(c ? c->getMaximumWidth() : -1) + ")");

        editor->setSize(700, 1424);
        check(std::abs(proc.getEditorScale() - 2.0f) < 1.0e-4f, "resizing updates the saved scale");

        juce::MemoryBlock state;
        proc.getStateInformation(state);
        NightDriveProcessor restored;
        restored.setStateInformation(state.getData(), (int)state.getSize());
        check(std::abs(restored.getEditorScale() - 2.0f) < 1.0e-4f, "the size is saved with the session");

        editor->setSize(525, 1068);
        if (snapshotDir != juce::File()) {
            auto img = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
            auto file = snapshotDir.getChildFile("NightDrive_150.png");
            file.deleteFile();
            juce::FileOutputStream out(file);
            juce::PNGImageFormat().writeImageToStream(img, out);
        }
    }
    {
        currentPlugin = "fonts";
        // What text actually renders with (JUCE resolves fonts through the default LookAndFeel)
        auto resolved = [] (int style) {
            auto t = juce::Font(12.0f, style).getTypefacePtr();
            return t != nullptr ? t->getName() + " " + t->getStyle() : juce::String("none");
        };
        {
            ff360_ui::FF360_LookAndFeel lnf; // as held by an open editor
            check(resolved(juce::Font::plain).startsWith("Barlow Condensed"), "text renders in Barlow Condensed (got " + resolved(juce::Font::plain) + ")");
            check(resolved(juce::Font::bold).containsIgnoreCase("Barlow Condensed SemiBold"), "bold text renders in Barlow Condensed SemiBold (got " + resolved(juce::Font::bold) + ")");
        }
        check(!resolved(juce::Font::plain).startsWith("Barlow"), "the default goes back once no editor is open");
    }
}

} // namespace

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI gui;

    juce::File snapshotDir;
    if (argc > 1) {
        snapshotDir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        snapshotDir.createDirectory();
    }

    testOutputStage();
    testOversampling();
    testDelta();
    testScalingAndFonts(snapshotDir);
    testPlugin<VHSPluginProcessor>("VHS", ff360::getVhsPresets(), snapshotDir);
    testPlugin<NeonChorusProcessor>("Neon Chorus", ff360::getNeonChorusPresets(), snapshotDir);
    testPlugin<MidnightReverbProcessor>("Midnight Reverb", ff360::getMidnightReverbPresets(), snapshotDir);
    testPlugin<NeonWidthProcessor>("Neon Width", ff360::getNeonWidthPresets(), snapshotDir);
    testPlugin<NeonTapeStopProcessor>("Neon Tape Stop", ff360::getNeonTapeStopPresets(), snapshotDir);
    testPlugin<CyberpunkGlitchProcessor>("Cyberpunk Glitch", ff360::getCyberpunkGlitchPresets(), snapshotDir);
    testPlugin<RetroFXProcessor>("RetroFX", ff360::getRetroFXPresets(), snapshotDir);
    testPlugin<NightDriveProcessor>("NightDrive", ff360::getNightDrivePresets(), snapshotDir);

    // Plugin-specific behaviour
    {
        currentPlugin = "Neon Tape Stop";
        NeonTapeStopProcessor proc;
        auto* trigger = proc.getApvts().getParameter("trigger");
        edit(*trigger, 1.0f);
        proc.getPresetManager().loadPreset(1);
        check(trigger->getValue() > 0.5f, "a preset load doesn't release a held tape stop");
        proc.getHistory().commit();
        proc.getHistory().switchSlot();
        check(trigger->getValue() > 0.5f, "A/B doesn't touch the trigger");
    }
    {
        // A preset copied in one plugin must not paste into another
        currentPlugin = "clipboard";
        VHSPluginProcessor vhs;
        NeonChorusProcessor chorus;
        vhs.getPresetManager().copyToClipboard();
        check(!chorus.getPresetManager().pasteFromClipboard(), "another plugin's preset is rejected");
    }
    {
        // The DEGRADE hero knob follows the parameter (presets, undo, automation)
        currentPlugin = "VHS";
        VHSPluginProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
        auto* degrade = proc.getApvts().getParameter("degrade");
        edit(*degrade, 0.7f);
        ff360_ui::FF360_HeroKnob* knob = nullptr;
        for (auto* c : editor->getChildren())
            if (auto* k = dynamic_cast<ff360_ui::FF360_HeroKnob*>(c)) knob = k;
        check(knob != nullptr && near(knob->getValue(), 0.7f), "hero knob follows the parameter");
    }


    {
        // Readouts follow their parameter (they used to be fixed placeholder text)
        struct Readout { const char* plugin; const char* id; float plain; const char* expected; };
        auto checkReadout = [](juce::AudioProcessor& proc, const Readout& r) {
            currentPlugin = r.plugin;
            std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
            auto* apvts = dynamic_cast<juce::AudioProcessorValueTreeState*>(&proc) ; juce::ignoreUnused(apvts);
            auto* p = dynamic_cast<juce::RangedAudioParameter*>(proc.getParameters()[0]); juce::ignoreUnused(p);
            for (auto* param : proc.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param); rp != nullptr && rp->paramID == r.id)
                    edit(*rp, rp->convertTo0to1(r.plain));
            check(labelTexts(*editor).contains(r.expected), juce::String("readout shows ") + r.expected + " for " + r.id);
        };
        CyberpunkGlitchProcessor glitch;   checkReadout(glitch, { "Cyberpunk Glitch", "probability", 25.0f, "25%" });
        CyberpunkGlitchProcessor glitch2;  checkReadout(glitch2, { "Cyberpunk Glitch", "filter", 2500.0f, "2.5 kHz" });
        NeonWidthProcessor width;          checkReadout(width, { "Neon Width", "bassmono", 200.0f, "200 Hz" });
        NightDriveProcessor nd1;           checkReadout(nd1, { "NightDrive", "evolve", 70.0f, "70%" });
        NightDriveProcessor nd2;           checkReadout(nd2, { "NightDrive", "mix", 55.0f, "55%" });
        NightDriveProcessor nd3;           checkReadout(nd3, { "NightDrive", "scduck", 40.0f, "40%" });
        VHSPluginProcessor vhs;            checkReadout(vhs, { "VHS", "mix", 60.0f, "60%" });
    }
    {
        // Value popups carry units
        currentPlugin = "Midnight Reverb";
        MidnightReverbProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
        std::vector<juce::Component*> controls;
        collectControls(*editor, controls);
        juce::StringArray texts;
        for (auto* c : controls)
            if (auto* sl = dynamic_cast<juce::Slider*>(c)) texts.add(sl->getTextFromValue(sl->getValue()));
        check(texts.contains("8.0 kHz"), "High Damp popup reads 8.0 kHz (got: " + texts.joinIntoString(", ") + ")");
        check(texts.contains("20.0 ms"), "Pre-delay popup reads 20.0 ms");
    }
    {
        // GENERATE FX makes sound (the button used to do nothing)
        currentPlugin = "RetroFX";
        RetroFXProcessor proc;
        proc.setRateAndBufferSizeDetails(48000.0, 512);
        proc.prepareToPlay(48000.0, 512);
        std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
        juce::AudioBuffer<float> buf(2, 512);
        juce::MidiBuffer midi;
        auto peakOf = [&](int blocks) {
            float peak = 0.0f;
            for (int b = 0; b < blocks; ++b) {
                buf.clear();
                proc.processBlock(buf, midi);
                peak = std::max(peak, buf.getMagnitude(0, buf.getNumSamples()));
            }
            return peak;
        };
        check(peakOf(10) < 1.0e-4f, "silent before GENERATE");
        std::vector<juce::Component*> controls;
        collectControls(*editor, controls);
        juce::Button* generate = nullptr;
        for (auto* c : controls)
            if (auto* b = dynamic_cast<juce::Button*>(c); b != nullptr && b->getButtonText() == "GENERATE FX") generate = b;
        check(generate != nullptr && generate->onClick != nullptr, "GENERATE FX has a click handler");
        if (generate != nullptr && generate->onClick) generate->onClick();
        check(peakOf(420) > 0.05f, "GENERATE FX produces sound"); // default: 4 s riser, quiet at first
    }

    std::cout << "\n" << (checks - failures) << " / " << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
