#pragma once

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ff360_ui {

// A parameter value as the UI shows it, from the parameter's unit label:
// "45%", "1.2 kHz", "350 Hz", "12.5 ms", "0.80 s", "+3 st", "-2.0 dB", "30 deg"
inline juce::String formatParameterValue(const juce::RangedAudioParameter& p, double value) {
    const auto label = p.getLabel();
    const auto sign = [value] { return value > 0.0 ? juce::String("+") : juce::String(); };

    if (label == "%")   return juce::String(juce::roundToInt(value)) + "%";
    if (label == "Hz")  return value >= 1000.0 ? juce::String(value / 1000.0, 1) + " kHz"
                                               : juce::String(juce::roundToInt(value)) + " Hz";
    if (label == "ms")  return juce::String(value, 1) + " ms";
    if (label == "s")   return juce::String(value, 2) + " s";
    if (label == "st")  return sign() + juce::String(juce::roundToInt(value)) + " st";
    if (label == "dB")  return sign() + juce::String(value, 1) + " dB";
    if (label == "deg") return sign() + juce::String(juce::roundToInt(value)) + " deg";

    const auto text = p.getText(p.convertTo0to1((float)value), 0);
    return label.isEmpty() ? text : text + " " + label;
}

// Call after the slider's SliderAttachment exists: shows the value with its unit in a bubble
// while dragging (the knobs have no readout of their own) and in the host-style text entry
inline void showValuePopup(juce::Slider& slider, juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId, juce::Component* popupParent) {
    if (auto* p = apvts.getParameter(paramId))
        slider.textFromValueFunction = [p](double v) { return formatParameterValue(*p, v); };
    slider.setPopupDisplayEnabled(true, false, popupParent);
    slider.getProperties().set("ff360ShowsValue", true); // JUCE has no getter; lets tests check coverage
}

// Keeps a readout label next to a slider showing the slider's value (presets, undo, automation
// and drags all go through the slider, so this covers every way the value changes)
inline void bindValueLabel(juce::Slider& slider, juce::Label& label) {
    slider.onValueChange = [&slider, &label] {
        label.setText(slider.getTextFromValue(slider.getValue()), juce::dontSendNotification);
    };
    slider.onValueChange();
    slider.getProperties().set("ff360ShowsValue", true);
}

} // namespace ff360_ui

#endif
