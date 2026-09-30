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

template <typename Proc>
void testPlugin(const juce::String& name, const std::vector<ff360::Preset>& factory, const juce::File& snapshotDir) {
    currentPlugin = name;
    std::cout << name << "\n";

    Proc proc;
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
    edit(*mix, 0.6f);
    presets.copyToClipboard();
    edit(*mix, 0.1f);
    check(presets.pasteFromClipboard(), "pastes its own preset");
    check(near(mix->getValue(), 0.6f), "paste applies the copied values");

    // ---- Editor ----
    std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
    check(editor->getWidth() == 350 && editor->getHeight() == 676, "editor is 350 x 676");
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

} // namespace

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI gui;

    juce::File snapshotDir;
    if (argc > 1) {
        snapshotDir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        snapshotDir.createDirectory();
    }

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

    std::cout << "\n" << (checks - failures) << " / " << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
