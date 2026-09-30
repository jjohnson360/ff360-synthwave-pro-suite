#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <array>
#include <atomic>
#include <vector>

namespace ff360_ui {

// Undo/redo and A/B compare for a plugin's parameters (same behaviour as the Dynamic EQ).
//
// Undo works on snapshots of every parameter's normalised value. A snapshot is committed
// shortly after the UI finishes an edit gesture (drag released, menu picked, wheel stopped),
// so one user action = one undo step, and host automation - which doesn't send gestures -
// isn't recorded.
//
// A/B keeps two complete parameter sets; the inactive one is stored as a snapshot and is
// saved with the session.
//
// Owned by the processor (not the editor) so history survives closing the plugin window.
class EditHistory : private juce::AudioProcessorListener,
                    private juce::Timer {
public:
    // paramIdsExcludedFromUndo: not undone, but still part of A/B.
    // viewOnlyParamIds: performance/view state (e.g. a momentary trigger), never touched by undo or A/B.
    EditHistory(juce::AudioProcessor& p, juce::StringArray paramIdsExcludedFromUndo = {},
                juce::StringArray viewOnlyParamIds = {})
        : processor(p) {
        params = processor.getParameters();
        for (auto* param : params) {
            juce::String id;
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                id = withId->paramID;
            paramIds.add(id);
            viewOnly.push_back(viewOnlyParamIds.contains(id));
            excludedFromUndo.push_back(paramIdsExcludedFromUndo.contains(id) || viewOnly.back());
        }

        processor.addListener(this);
        resetNow();
        startTimerHz(30);
    }

    ~EditHistory() override {
        stopTimer();
        processor.removeListener(this);
    }

    // ---- Undo ----
    bool canUndo() const { return index > 0; }
    bool canRedo() const { return index + 1 < (int)states.size(); }

    void undo() {
        commit(); // don't lose an edit still waiting for its debounce
        if (!canUndo()) return;
        --index;
        apply(states[(size_t)index], false);
    }

    void redo() {
        if (!canRedo()) return;
        ++index;
        apply(states[(size_t)index], false);
    }

    // Record the current state now (for edits that don't send gestures, e.g. presets)
    void commit() {
        auto current = capture();
        if (index >= 0 && sameForUndo(current, states[(size_t)index]))
            return;

        // A new edit discards the redo branch
        states.resize((size_t)index + 1);
        states.push_back(std::move(current));
        if (states.size() > maxStates)
            states.erase(states.begin());
        index = (int)states.size() - 1;
    }

    // Forget the history and start fresh from the current state (e.g. after a session load).
    // Safe to call from any thread; the reset happens on the message thread.
    void requestReset() { resetRequested.store(true); }

    // ---- A/B compare ----
    int getActiveSlot() const { return activeSlot; }

    void switchSlot() {
        Snapshot target;
        {
            const juce::ScopedLock sl(slotLock);
            slots[(size_t)activeSlot] = capture();
            activeSlot ^= 1;

            // First visit to the other slot: it starts as a copy of the current one
            if (slots[(size_t)activeSlot].empty())
                slots[(size_t)activeSlot] = slots[(size_t)(activeSlot ^ 1)];
            target = slots[(size_t)activeSlot];
        }

        apply(target, true); // outside the lock: this notifies the host

        // Each slot is its own state; undoing across a switch would mix the two
        resetNow();
    }

    void copyActiveToOther() {
        auto current = capture();
        const juce::ScopedLock sl(slotLock);
        slots[(size_t)(activeSlot ^ 1)] = std::move(current);
    }

    // ---- Persistence of the A/B state ----
    void writeState(juce::XmlElement& xml) const {
        xml.setAttribute("abActive", activeSlot);
        xml.setAttribute("abInactive", serialiseInactiveSlot());
    }

    // Call after the parameters are restored; also starts a fresh undo history
    void readState(const juce::XmlElement& xml) {
        restoreSlots(xml.getIntAttribute("abActive", 0), xml.getStringAttribute("abInactive"));
        requestReset();
    }

private:
    using Snapshot = std::vector<float>;

    // Keyed by parameter ID so it survives parameters being added in later versions
    juce::String serialiseInactiveSlot() const {
        const juce::ScopedLock sl(slotLock);
        const auto& s = slots[(size_t)(activeSlot ^ 1)];
        if ((int)s.size() != params.size())
            return {};

        juce::StringArray entries;
        for (int i = 0; i < params.size(); ++i)
            if (paramIds[i].isNotEmpty() && !viewOnly[(size_t)i])
                entries.add(paramIds[i] + "=" + juce::String(s[(size_t)i], 7));
        return entries.joinIntoString(";");
    }

    void restoreSlots(int active, const juce::String& inactiveSlotData) {
        const juce::ScopedLock sl(slotLock);
        activeSlot = juce::jlimit(0, 1, active);
        slots[0].clear();
        slots[1].clear();

        if (inactiveSlotData.isEmpty())
            return;

        // Parameters missing from the saved data (e.g. added in a later version) use defaults
        Snapshot s;
        s.reserve((size_t)params.size());
        for (auto* param : params)
            s.push_back(param->getDefaultValue());

        for (const auto& entry : juce::StringArray::fromTokens(inactiveSlotData, ";", {})) {
            const int idx = paramIds.indexOf(entry.upToFirstOccurrenceOf("=", false, false));
            if (idx >= 0)
                s[(size_t)idx] = juce::jlimit(0.0f, 1.0f, entry.fromFirstOccurrenceOf("=", false, false).getFloatValue());
        }

        slots[(size_t)(activeSlot ^ 1)] = std::move(s);
    }

    Snapshot capture() const {
        Snapshot s;
        s.reserve((size_t)params.size());
        for (auto* param : params)
            s.push_back(param->getValue());
        return s;
    }

    void apply(const Snapshot& s, bool includeExcluded) {
        if ((int)s.size() != params.size())
            return;

        const juce::ScopedValueSetter<bool> guard(applying, true);
        for (int i = 0; i < params.size(); ++i) {
            if (viewOnly[(size_t)i] || (!includeExcluded && excludedFromUndo[(size_t)i]))
                continue;

            auto* param = params[i];
            const float v = s[(size_t)i];
            if (std::abs(param->getValue() - v) > 1.0e-7f) {
                param->beginChangeGesture();
                param->setValueNotifyingHost(v);
                param->endChangeGesture();
            }
        }
    }

    bool sameForUndo(const Snapshot& a, const Snapshot& b) const {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (!excludedFromUndo[i] && std::abs(a[i] - b[i]) > 1.0e-7f)
                return false;
        return true;
    }

    void resetNow() {
        states.clear();
        states.push_back(capture());
        index = 0;
        commitPending.store(false);
    }

    // AudioProcessorListener
    void audioProcessorParameterChanged(juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged(juce::AudioProcessor*, const ChangeDetails&) override {}

    void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*, int) override {
        if (applying) return;
        ++gestureDepth;
        lastGestureEndMs.store(juce::Time::getMillisecondCounter());
    }

    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor*, int) override {
        // May arrive on any thread: just flag it, the timer commits on the message thread
        if (applying) return;
        if (gestureDepth.load() > 0) --gestureDepth;
        lastGestureEndMs.store(juce::Time::getMillisecondCounter());
        commitPending.store(true);
    }

    // Timer (message thread): debounced commits and deferred resets
    void timerCallback() override {
        if (resetRequested.exchange(false)) {
            resetNow();
            return;
        }

        const auto sinceLast = juce::Time::getMillisecondCounter() - lastGestureEndMs.load();

        // Watchdog: a begin without a matching end (misbehaving caller) must not block history forever
        if (gestureDepth.load() > 0 && sinceLast > 10000)
            gestureDepth.store(0);

        if (commitPending.load() && gestureDepth.load() == 0 && sinceLast >= kCoalesceMs) {
            commitPending.store(false);
            commit();
        }
    }

    // Gestures ending within this window are merged into one undo step (e.g. a mouse-wheel
    // spin sends a gesture per tick)
    static constexpr juce::uint32 kCoalesceMs = 250;
    static constexpr size_t maxStates = 100;

    juce::AudioProcessor& processor;
    juce::Array<juce::AudioProcessorParameter*> params;
    juce::StringArray paramIds;
    std::vector<bool> excludedFromUndo;
    std::vector<bool> viewOnly;

    std::vector<Snapshot> states;
    int index = -1;

    std::atomic<bool> commitPending { false };
    std::atomic<juce::uint32> lastGestureEndMs { 0 };
    std::atomic<bool> resetRequested { false };
    std::atomic<int> gestureDepth { 0 }; // edits in progress; no commits until they all end
    bool applying = false;

    std::array<Snapshot, 2> slots;
    int activeSlot = 0;
    juce::CriticalSection slotLock; // hosts may save state from a non-message thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditHistory)
};

} // namespace ff360_ui

#endif
