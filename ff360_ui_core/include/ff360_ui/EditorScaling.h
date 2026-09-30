#pragma once

#if __has_include(<juce_audio_processors/juce_audio_processors.h>)
#include <juce_audio_processors/juce_audio_processors.h>

namespace ff360_ui {

// Resizable editors for the Synthwave plugins: every editor lays out and paints at one design
// size, and the whole UI is scaled uniformly (aspect ratio locked, 75% to 200%). Everything is
// drawn with paths and text, so it stays sharp at any size.
//
//   constructor:  EditorScaling::setup(*this, processor.getEditorScale())   (instead of setSize)
//   paint:        g.addTransform(EditorScaling::transformFor(*this)); then draw in designBounds()
//   resized:      lay out in designBounds(), then EditorScaling::applyToChildren(*this)
struct EditorScaling {
    static constexpr int designWidth = 350;
    static constexpr int designHeight = 712;
    static constexpr float minScale = 0.75f;
    static constexpr float maxScale = 2.0f;

    static void setup(juce::AudioProcessorEditor& editor, float initialScale) {
        editor.setResizable(true, true);
        editor.setResizeLimits(juce::roundToInt(designWidth * minScale), juce::roundToInt(designHeight * minScale),
                               juce::roundToInt(designWidth * maxScale), juce::roundToInt(designHeight * maxScale));
        if (auto* c = editor.getConstrainer())
            c->setFixedAspectRatio((double)designWidth / (double)designHeight);
        const float s = juce::jlimit(minScale, maxScale, initialScale);
        editor.setSize(juce::roundToInt(designWidth * s), juce::roundToInt(designHeight * s));
    }

    static juce::Rectangle<int> designBounds() { return { 0, 0, designWidth, designHeight }; }

    static float scaleOf(const juce::Component& editor) {
        return juce::jlimit(minScale, maxScale, (float)editor.getWidth() / (float)designWidth);
    }

    static juce::AffineTransform transformFor(const juce::Component& editor) {
        return juce::AffineTransform::scale(scaleOf(editor));
    }

    // Scales every child laid out in design coordinates. The host resize corner and the tooltip
    // window position themselves in real pixels, so they're left alone.
    static void applyToChildren(juce::Component& editor) {
        const auto t = transformFor(editor);
        for (auto* c : editor.getChildren())
            if (dynamic_cast<juce::ResizableCornerComponent*>(c) == nullptr && dynamic_cast<juce::TooltipWindow*>(c) == nullptr
                && dynamic_cast<juce::BubbleComponent*>(c) == nullptr)
                c->setTransform(t);
    }
};

} // namespace ff360_ui

#endif
