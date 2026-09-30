#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ff360/Preset.h"
#include "EditHistory.h"
#include <vector>

namespace ff360_ui {

// Factory + user presets (the house preset menu shared with the Dynamic EQ / Limiter / Niche360).
//
// The list is "Default" (every parameter at its default), then the plugin's factory presets
// (Presets.h, grouped by category), then user presets: XML files in the user preset folder,
// sorted by name. A preset stores every parameter except performance/view ones (e.g. a momentary
// trigger). Parameters a preset doesn't mention get their defaults, so a preset always sounds the
// same whatever was loaded before. The same XML is used for the clipboard (Copy / Paste).
//
// The current preset is tracked by name, and is "modified" when any preset parameter differs
// from the values right after it was loaded.
class PresetManager {
public:
    struct Entry {
        juce::String name;
        juce::String category; // factory submenu; empty = top level (Default and all user presets)
        bool isUser = false;
        juce::File file;       // user presets only
        int factoryIndex = -1; // index into the factory list; -1 = Default
    };

    // productName: the user preset folder, e.g. "VHS" -> <app data>/ff360 Labs/Synthwave/VHS/Presets
    // nonPresetParamIds: parameters a preset never stores or changes
    PresetManager(juce::AudioProcessorValueTreeState& s, EditHistory& h, juce::String productName,
                  std::vector<ff360::Preset> factoryPresets, juce::StringArray nonPresetParamIds = {})
        : apvts(s), history(h), product(std::move(productName)), factory(std::move(factoryPresets)) {
        for (auto* p : apvts.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                if (!nonPresetParamIds.contains(ranged->paramID))
                    presetParams.add(ranged);

        for (const auto& preset : factory)
            categories.addIfNotAlreadyThere(juce::String::fromUTF8(preset.category.c_str()));

        scanUserPresets();

        // A fresh instance starts on the default setting
        currentName = defaultName;
        loadedValues = capture();
    }

    // ---- List ----
    void scanUserPresets() {
        entries.clear();
        entries.push_back({ defaultName, {}, false, {}, -1 });
        for (int i = 0; i < (int)factory.size(); ++i)
            entries.push_back({ juce::String::fromUTF8(factory[(size_t)i].name.c_str()),
                                juce::String::fromUTF8(factory[(size_t)i].category.c_str()), false, {}, i });

        std::vector<Entry> user;
        for (const auto& f : getUserPresetFolder().findChildFiles(juce::File::findFiles, false, "*.xml"))
            if (auto xml = juce::XmlDocument::parse(f); xml != nullptr && xml->hasTagName(presetTag))
                user.push_back({ f.getFileNameWithoutExtension(), {}, true, f, -1 });

        std::sort(user.begin(), user.end(), [](const Entry& a, const Entry& b)
                  { return a.name.compareNatural(b.name) < 0; });
        entries.insert(entries.end(), user.begin(), user.end());
    }

    int getNumPresets() const { return (int)entries.size(); }
    const Entry& getEntry(int i) const { return entries[(size_t)i]; }
    const juce::StringArray& getCategories() const { return categories; }

    juce::String getDisplayName(int i) const {
        const auto& e = getEntry(i);
        return e.isUser ? juce::String::fromUTF8("\xE2\x98\x85 ") + e.name : e.name; // star
    }

    // ---- Current preset ----
    // -1 when the current settings aren't in the list (e.g. pasted)
    int getCurrentIndex() const {
        for (int i = 0; i < getNumPresets(); ++i)
            if (entries[(size_t)i].isUser == currentIsUser && entries[(size_t)i].name == currentName)
                return i;
        return -1;
    }

    juce::String getCurrentName() const { return currentName; }
    bool isCurrentUserPreset() const { return currentIsUser; }

    bool isModified() const {
        if (modifiedAtRestore)
            return true;
        if ((int)loadedValues.size() != presetParams.size())
            return false;
        for (int i = 0; i < presetParams.size(); ++i)
            if (std::abs(presetParams[i]->getValue() - loadedValues[(size_t)i]) > 1.0e-4f)
                return true;
        return false;
    }

    // Loading applies the values, notifies the host and makes one undo step
    void loadPreset(int i) {
        if (i < 0 || i >= getNumPresets())
            return;

        const auto e = getEntry(i); // copy: a failed user preset read rescans the list
        if (!e.isUser) {
            applyFactory(e.factoryIndex);
        } else {
            auto xml = juce::XmlDocument::parse(e.file);
            if (xml == nullptr || !applyPresetXml(*xml)) {
                scanUserPresets(); // file was removed or damaged outside the plugin
                return;
            }
        }
        finishLoad(e.name, e.isUser);
    }

    // Step through the list, wrapping (arrows in the preset bar)
    void loadNext(int delta) {
        const int n = getNumPresets();
        if (n == 0) return;
        const int cur = getCurrentIndex();
        loadPreset(cur < 0 ? (delta > 0 ? 0 : n - 1) : ((cur + delta) % n + n) % n);
    }

    // ---- User presets ----
    juce::File getUserPresetFolder() const {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("ff360 Labs")
                       .getChildFile("Synthwave")
                       .getChildFile(product)
                       .getChildFile("Presets");
        if (!dir.isDirectory())
            dir.createDirectory();
        return dir;
    }

    bool userPresetExists(const juce::String& name) const {
        return getUserPresetFolder().getChildFile(juce::File::createLegalFileName(name.trim()) + ".xml").existsAsFile();
    }

    // Overwrites a user preset of the same name
    void saveUserPreset(const juce::String& name) {
        auto clean = juce::File::createLegalFileName(name.trim());
        if (clean.isEmpty())
            clean = "User Preset";

        createPresetXml(clean)->writeTo(getUserPresetFolder().getChildFile(clean + ".xml"));
        scanUserPresets();

        // The saved preset is now the current one, unmodified (no undo step: nothing changed)
        currentName = clean;
        currentIsUser = true;
        modifiedAtRestore = false;
        loadedValues = capture();
    }

    void deleteUserPreset(int i) {
        if (i < 0 || i >= getNumPresets() || !getEntry(i).isUser)
            return;

        // The current settings stay as they are; if they came from this preset, they no longer
        // match a saved one
        if (currentIsUser && getEntry(i).name == currentName)
            currentIsUser = false;

        getEntry(i).file.deleteFile();
        scanUserPresets();
    }

    // ---- Clipboard ----
    void copyToClipboard() const {
        juce::SystemClipboard::copyTextToClipboard(createPresetXml(currentName)->toString());
    }

    // false if the clipboard doesn't hold a preset for this plugin
    bool pasteFromClipboard() {
        auto xml = juce::XmlDocument::parse(juce::SystemClipboard::getTextFromClipboard());
        if (xml == nullptr || !applyPresetXml(*xml))
            return false;

        // Shows the copied preset's name; it's not tied to a saved preset file
        finishLoad(xml->getStringAttribute("name", "Pasted"), false);
        return true;
    }

    // ---- Session state (current preset name and whether it was modified) ----
    void writeState(juce::XmlElement& xml) const {
        xml.setAttribute("presetName", currentName);
        xml.setAttribute("presetIsUser", currentIsUser);
        xml.setAttribute("presetModified", isModified());
    }

    // Call after the parameters are restored
    void readState(const juce::XmlElement& xml) {
        currentName = xml.getStringAttribute("presetName", defaultName);
        currentIsUser = xml.getBoolAttribute("presetIsUser", false);
        // Sessions from before presets were tracked: we can't know, so call them modified
        modifiedAtRestore = xml.getBoolAttribute("presetModified", !xml.hasAttribute("presetName"));
        loadedValues = capture();
    }

private:
    using Snapshot = std::vector<float>;

    Snapshot capture() const {
        Snapshot s;
        s.reserve((size_t)presetParams.size());
        for (auto* p : presetParams)
            s.push_back(p->getValue());
        return s;
    }

    // Default (-1) or a factory preset: its JSON "parameters" object holds plain values
    void applyFactory(int factoryIndex) {
        juce::var values;
        if (factoryIndex >= 0 && factoryIndex < (int)factory.size())
            values = juce::JSON::parse(juce::String::fromUTF8(factory[(size_t)factoryIndex].jsonContent.c_str()))
                         .getProperty("parameters", {});

        for (auto* p : presetParams) {
            float normalised = p->getDefaultValue();
            if (auto* obj = values.getDynamicObject(); obj != nullptr && obj->hasProperty(p->paramID))
                normalised = p->convertTo0to1((float)obj->getProperty(p->paramID));
            setNormalised(*p, normalised);
        }
    }

    std::unique_ptr<juce::XmlElement> createPresetXml(const juce::String& name) const {
        auto xml = std::make_unique<juce::XmlElement>(presetTag);
        xml->setAttribute("name", name);
        xml->setAttribute("plugin", product);
        xml->setAttribute("version", 1);
        for (auto* p : presetParams) {
            auto* e = xml->createNewChildElement("PARAM");
            e->setAttribute("id", p->paramID);
            e->setAttribute("value", p->convertFrom0to1(p->getValue()));
        }
        return xml;
    }

    bool applyPresetXml(const juce::XmlElement& xml) {
        // Every Synthwave plugin shares the tag, so also check it's this plugin's preset
        if (!xml.hasTagName(presetTag) || xml.getStringAttribute("plugin", product) != product)
            return false;

        for (auto* p : presetParams) {
            float normalised = p->getDefaultValue();
            if (auto* e = xml.getChildByAttribute("id", p->paramID))
                normalised = p->convertTo0to1((float)e->getDoubleAttribute("value"));
            setNormalised(*p, normalised);
        }
        return true;
    }

    static void setNormalised(juce::RangedAudioParameter& p, float normalised) {
        normalised = juce::jlimit(0.0f, 1.0f, normalised);
        if (std::abs(p.getValue() - normalised) > 1.0e-7f)
            p.setValueNotifyingHost(normalised);
    }

    void finishLoad(const juce::String& name, bool isUser) {
        currentName = name;
        currentIsUser = isUser;
        modifiedAtRestore = false;
        loadedValues = capture();
        history.commit(); // a preset load is one undoable step
    }

    static constexpr const char* presetTag = "FF360SynthwavePreset";
    static constexpr const char* defaultName = "Default";

    juce::AudioProcessorValueTreeState& apvts;
    EditHistory& history;
    juce::String product;
    std::vector<ff360::Preset> factory;
    juce::StringArray categories; // in order of first appearance

    juce::Array<juce::RangedAudioParameter*> presetParams; // parameters a preset stores
    std::vector<Entry> entries;

    juce::String currentName;
    bool currentIsUser = false;
    Snapshot loadedValues;          // preset parameter values right after the last load
    bool modifiedAtRestore = false; // restored sessions remember "modified" until the next load

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace ff360_ui

#endif
