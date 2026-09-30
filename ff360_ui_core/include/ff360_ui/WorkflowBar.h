#pragma once

#include "DesignTokens.h"
#include "Fonts.h"

#if __has_include(<juce_gui_basics/juce_gui_basics.h>)
#include <juce_gui_basics/juce_gui_basics.h>
#include "EditHistory.h"
#include "PresetManager.h"

namespace ff360_ui {

// Header workflow row shared by every Synthwave plugin:
//
//   [undo][redo]   [‹  Preset Name •  ▾  ›]   [A|B][A→B]
//
// Clicking the preset name opens the house preset menu (Default, factory presets in category
// submenus, user presets, Copy / Paste / Save As..., Options). A gold dot after the name shows
// the settings were changed since the preset was loaded. Undo/redo and A/B work on the
// processor-owned EditHistory, so they survive closing the plugin window.
class WorkflowBar : public juce::Component,
                    private juce::KeyListener,
                    private juce::Timer {
public:
    WorkflowBar(EditHistory& h, PresetManager& pm) : history(h), presets(pm) {
        for (auto* b : { &btnUndo, &btnRedo, &btnPrev, &btnNext, &btnSlotA, &btnSlotB, &btnCopy })
            addAndMakeVisible(*b);

        btnUndo.setTooltip("Undo (Ctrl+Z)");
        btnRedo.setTooltip("Redo (Ctrl+Shift+Z / Ctrl+Y)");
        btnPrev.setTooltip("Previous preset");
        btnNext.setTooltip("Next preset");
        btnSlotA.setTooltip("Settings A");
        btnSlotB.setTooltip("Settings B");
        btnCopy.setTooltip("Copy these settings to the other slot");

        btnUndo.onClick = [this] { history.undo(); changed(); };
        btnRedo.onClick = [this] { history.redo(); changed(); };
        btnPrev.onClick = [this] { presets.loadNext(-1); changed(); };
        btnNext.onClick = [this] { presets.loadNext(1); changed(); };
        btnSlotA.onClick = [this] { if (history.getActiveSlot() != 0) history.switchSlot(); changed(); };
        btnSlotB.onClick = [this] { if (history.getActiveSlot() != 1) history.switchSlot(); changed(); };
        btnCopy.onClick = [this] { history.copyActiveToOther(); };

        refresh();
        startTimerHz(10); // picks up edits made elsewhere (knobs, automation, host undo)
    }

    ~WorkflowBar() override {
        if (keyTarget != nullptr)
            keyTarget->removeKeyListener(this);
    }

    // Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y while the plugin window has focus. Many hosts keep these
    // keys for themselves, which is why the buttons exist too.
    void attachKeyboardShortcuts(juce::Component& editor) {
        keyTarget = &editor;
        editor.setWantsKeyboardFocus(true);
        editor.addKeyListener(this);
    }

    // Called after anything that changes the parameters from the bar (load, paste, undo, A/B)
    std::function<void()> onStateChanged;

    void paint(juce::Graphics& g) override {
        // Preset box
        auto box = presetBox.toFloat();
        g.setColour(juce::Colour(Colors::DeepBlack));
        g.fillRoundedRectangle(box, 6.0f);
        g.setColour(juce::Colour(Colors::MetallicGold).withAlpha(0.22f));
        g.drawRoundedRectangle(box.reduced(0.5f), 6.0f, 1.0f);

        if (nameHover) {
            g.setColour(juce::Colour(Colors::Charcoal2));
            g.fillRect(nameArea.reduced(0, 1));
        }

        // Name, then the modified dot and a dropdown caret, centred together
        const juce::String star = presets.isCurrentUserPreset() ? juce::String::fromUTF8("\xE2\x98\x85 ") : juce::String();
        const juce::Font font(11.5f, juce::Font::bold);
        const float dotW = shownModified ? 10.0f : 0.0f, caretW = 13.0f;
        const auto area = nameArea.toFloat().reduced(3.0f, 0.0f);
        const float maxTextW = juce::jmax(10.0f, area.getWidth() - dotW - caretW);
        const float textW = juce::jmin(maxTextW, font.getStringWidthFloat(star + shownName) + 1.0f);
        float x = area.getCentreX() - (textW + dotW + caretW) * 0.5f;
        const float cy = area.getCentreY();

        g.setFont(font);
        g.setColour(juce::Colour(Colors::TextOffWhite));
        g.drawFittedText(star + shownName, juce::Rectangle<float>(x, area.getY(), textW, area.getHeight()).toNearestInt(),
                         juce::Justification::centredLeft, 1, 0.8f);
        x += textW;

        if (shownModified) {
            g.setColour(juce::Colour(Colors::MetallicGold));
            g.fillEllipse(x + 3.5f, cy - 2.5f, 5.0f, 5.0f);
            x += dotW;
        }

        juce::Path caret;
        caret.addTriangle(x + 4.0f, cy - 2.0f, x + 11.0f, cy - 2.0f, x + 7.5f, cy + 2.5f);
        g.setColour(nameHover ? juce::Colour(Colors::MetallicGold) : juce::Colour(Colors::TextDim));
        g.fillPath(caret);
    }

    void resized() override {
        constexpr int btn = 22, gap = 2, groupGap = 6;
        auto b = getLocalBounds();
        const int y = (b.getHeight() - btn) / 2;

        btnUndo.setBounds(b.getX(), y, btn, btn);
        btnRedo.setBounds(b.getX() + btn + gap, y, btn, btn);
        b.removeFromLeft(2 * btn + gap + groupGap);

        btnCopy.setBounds(b.getRight() - btn, y, btn, btn);
        btnSlotB.setBounds(b.getRight() - 2 * btn - gap, y, btn, btn);
        btnSlotA.setBounds(b.getRight() - 3 * btn - gap, y, btn, btn);
        b.removeFromRight(3 * btn + gap + groupGap);

        presetBox = b;
        btnPrev.setBounds(b.removeFromLeft(btn).withSizeKeepingCentre(btn, btn));
        btnNext.setBounds(b.removeFromRight(btn).withSizeKeepingCentre(btn, btn));
        nameArea = b;
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const bool over = nameArea.contains(e.getPosition());
        if (over != nameHover) {
            nameHover = over;
            setMouseCursor(over ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        if (nameHover) {
            nameHover = false;
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (e.mouseWasClicked() && nameArea.contains(e.getPosition()))
            showPresetMenu();
    }

private:
    // Small flat button whose icon is drawn as a path (no font glyph dependency)
    class IconButton : public juce::Button {
    public:
        enum class Icon { Undo, Redo, Prev, Next, Copy, Text };

        IconButton(Icon i, const juce::String& text = {}) : juce::Button(text), icon(i) {}

        void setActive(bool shouldBeActive) {
            if (active != shouldBeActive) {
                active = shouldBeActive;
                repaint();
            }
        }

        void paintButton(juce::Graphics& g, bool highlighted, bool down) override {
            auto r = getLocalBounds().toFloat().reduced(0.5f);
            const auto gold = juce::Colour(Colors::MetallicGold);

            if (active) {
                g.setColour(gold.withAlpha(0.14f));
                g.fillRoundedRectangle(r, 5.0f);
            } else if (highlighted || down) {
                g.setColour(gold.withAlpha(down ? 0.12f : 0.06f));
                g.fillRoundedRectangle(r, 5.0f);
            }
            if (icon != Icon::Prev && icon != Icon::Next) {
                g.setColour(active ? gold : juce::Colour(0x24FFFFFF));
                g.drawRoundedRectangle(r, 5.0f, 1.0f);
            }

            auto c = active || highlighted ? gold : juce::Colour(Colors::TextDim);
            if (!isEnabled())
                c = c.withAlpha(0.3f);
            g.setColour(c);

            const float cx = r.getCentreX(), cy = r.getCentreY();
            const juce::PathStrokeType stroke(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            juce::Path p;

            switch (icon) {
                case Icon::Undo:
                case Icon::Redo: {
                    // Arc over the top from the start side (arrowhead, pointing down) to the other side
                    const float dir = icon == Icon::Redo ? 1.0f : -1.0f;
                    const float rad = 5.0f, acy = cy + 1.5f;
                    const float halfPi = juce::MathConstants<float>::halfPi;
                    if (icon == Icon::Undo)
                        p.addCentredArc(cx, acy, rad, rad, 0.0f, -halfPi, halfPi * 1.2f, true);
                    else
                        p.addCentredArc(cx, acy, rad, rad, 0.0f, -halfPi * 1.2f, halfPi, true);
                    g.strokePath(p, stroke);
                    const float ax = cx + dir * rad;
                    juce::Path head;
                    head.addTriangle(ax - 3.0f, acy - 0.5f, ax + 3.0f, acy - 0.5f, ax, acy + 3.5f);
                    g.fillPath(head);
                    break;
                }
                case Icon::Prev:
                case Icon::Next: {
                    const float dir = icon == Icon::Next ? 1.0f : -1.0f;
                    p.startNewSubPath(cx - dir * 2.0f, cy - 4.0f);
                    p.lineTo(cx + dir * 2.0f, cy);
                    p.lineTo(cx - dir * 2.0f, cy + 4.0f);
                    g.strokePath(p, stroke);
                    break;
                }
                case Icon::Copy: {
                    // Arrow pointing at the inactive slot's side
                    const float dir = pointsLeft ? -1.0f : 1.0f;
                    p.startNewSubPath(cx - dir * 5.0f, cy);
                    p.lineTo(cx + dir * 4.0f, cy);
                    p.startNewSubPath(cx + dir * 1.0f, cy - 3.0f);
                    p.lineTo(cx + dir * 4.0f, cy);
                    p.lineTo(cx + dir * 1.0f, cy + 3.0f);
                    g.strokePath(p, stroke);
                    break;
                }
                case Icon::Text:
                    g.setFont(ff360_ui::brandFont(10.5f, juce::Font::bold));
                    g.drawText(getButtonText(), r, juce::Justification::centred, false);
                    break;
            }
        }

        bool pointsLeft = false; // Copy icon: B -> A

    private:
        Icon icon;
        bool active = false;
    };

    void changed() {
        refresh();
        if (onStateChanged)
            onStateChanged();
    }

    void refresh() {
        btnUndo.setEnabled(history.canUndo());
        btnRedo.setEnabled(history.canRedo());

        const int slot = history.getActiveSlot();
        btnSlotA.setActive(slot == 0);
        btnSlotB.setActive(slot == 1);
        if (btnCopy.pointsLeft != (slot == 1)) {
            btnCopy.pointsLeft = slot == 1;
            btnCopy.setTooltip(slot == 0 ? "Copy A to B" : "Copy B to A");
            btnCopy.repaint();
        }

        const auto name = presets.getCurrentName();
        const bool modified = presets.isModified();
        if (name != shownName || modified != shownModified) {
            shownName = name;
            shownModified = modified;
            repaint();
        }
    }

    void timerCallback() override { refresh(); }

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override {
        const auto mods = key.getModifiers();
        if (mods.isCommandDown() && key.getKeyCode() == 'Z') {
            if (mods.isShiftDown()) history.redo();
            else                    history.undo();
            changed();
            return true;
        }
        if (mods.isCommandDown() && key.getKeyCode() == 'Y') {
            history.redo();
            changed();
            return true;
        }
        return false;
    }

    void showPresetMenu() {
        enum { copyId = 10000, pasteId, saveId, deleteId, folderId, rescanId };

        presets.scanUserPresets(); // pick up files added or removed outside the plugin
        const int cur = presets.getCurrentIndex();

        juce::PopupMenu menu;
        menu.setLookAndFeel(&getLookAndFeel());

        // Default first, then factory presets in category submenus (the one holding the
        // current preset is ticked), then user presets at the top level
        for (int i = 0; i < presets.getNumPresets(); ++i)
            if (!presets.getEntry(i).isUser && presets.getEntry(i).category.isEmpty())
                menu.addItem(i + 1, presets.getDisplayName(i), true, i == cur);

        for (const auto& category : presets.getCategories()) {
            juce::PopupMenu sub;
            bool containsCurrent = false;
            for (int i = 0; i < presets.getNumPresets(); ++i) {
                const auto& e = presets.getEntry(i);
                if (e.isUser || e.category != category)
                    continue;
                sub.addItem(i + 1, e.name, true, i == cur);
                containsCurrent = containsCurrent || i == cur;
            }
            if (sub.getNumItems() > 0)
                menu.addSubMenu(category, sub, true, nullptr, containsCurrent);
        }

        bool firstUser = true;
        for (int i = 0; i < presets.getNumPresets(); ++i) {
            if (!presets.getEntry(i).isUser)
                continue;
            if (firstUser) {
                menu.addSeparator();
                firstUser = false;
            }
            menu.addItem(i + 1, presets.getDisplayName(i), true, i == cur);
        }

        menu.addSeparator();
        menu.addItem(copyId, "Copy");
        menu.addItem(pasteId, "Paste");
        menu.addItem(saveId, "Save As...");

        juce::PopupMenu options;
        const bool canDelete = cur >= 0 && presets.getEntry(cur).isUser;
        options.addItem(deleteId, canDelete ? "Delete \"" + presets.getEntry(cur).name + "\"" : juce::String("Delete Preset"), canDelete);
        options.addItem(folderId, "Show Preset Folder");
        options.addItem(rescanId, "Refresh User Presets");
        menu.addSubMenu("Options", options);

        menu.showMenuAsync(juce::PopupMenu::Options()
                               .withTargetComponent(this)
                               .withTargetScreenArea(localAreaToGlobal(presetBox))
                               .withMinimumWidth(presetBox.getWidth())
                               .withStandardItemHeight(22),
                           [safeThis = juce::Component::SafePointer<WorkflowBar>(this), cur](int result) {
                               if (safeThis == nullptr || result == 0)
                                   return;
                               auto& pm = safeThis->presets;

                               if (result == copyId)
                                   pm.copyToClipboard();
                               else if (result == pasteId) {
                                   if (pm.pasteFromClipboard())
                                       safeThis->changed();
                               } else if (result == saveId)
                                   safeThis->promptSave();
                               else if (result == deleteId)
                                   safeThis->promptDelete(cur);
                               else if (result == folderId)
                                   pm.getUserPresetFolder().startAsProcess();
                               else if (result == rescanId)
                                   pm.scanUserPresets();
                               else if (result > 0 && result <= pm.getNumPresets()) {
                                   pm.loadPreset(result - 1);
                                   safeThis->changed();
                               }
                           });
    }

    void promptSave() {
        // Suggest the current name for user presets (re-save), "<name> Copy" for factory ones
        auto suggestion = presets.getCurrentName();
        if (!presets.isCurrentUserPreset())
            suggestion += " Copy";

        auto* alert = new juce::AlertWindow("Save Preset", "Enter a name for this preset:", juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor("name", suggestion);
        alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        juce::Component::SafePointer<WorkflowBar> safeThis(this);
        alert->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, alert](int result) {
            if (safeThis == nullptr || result != 1)
                return;

            const auto name = alert->getTextEditorContents("name").trim();
            if (name.isEmpty())
                return;

            auto save = [safeThis, name] {
                if (safeThis == nullptr) return;
                safeThis->presets.saveUserPreset(name);
                safeThis->refresh();
            };

            // Re-saving the current user preset overwrites silently; any other existing name asks first
            const bool resaving = safeThis->presets.isCurrentUserPreset()
                                  && juce::File::createLegalFileName(name) == safeThis->presets.getCurrentName();
            if (!safeThis->presets.userPresetExists(name) || resaving) {
                save();
                return;
            }

            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Replace Preset",
                                               "A user preset named \"" + name + "\" already exists. Replace it?",
                                               "Replace", "Cancel", safeThis.getComponent(),
                                               juce::ModalCallbackFunction::create([save](int ok) { if (ok != 0) save(); }));
        }), true);
    }

    void promptDelete(int i) {
        if (i < 0 || i >= presets.getNumPresets() || !presets.getEntry(i).isUser)
            return;

        const auto name = presets.getEntry(i).name;
        juce::Component::SafePointer<WorkflowBar> safeThis(this);
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Delete Preset",
                                           "Delete the user preset \"" + name + "\"? This can't be undone.",
                                           "Delete", "Cancel", this,
                                           juce::ModalCallbackFunction::create([safeThis, name](int ok) {
                                               if (safeThis == nullptr || ok == 0)
                                                   return;
                                               // Find it again by name: the list may have changed meanwhile
                                               auto& pm = safeThis->presets;
                                               for (int j = 0; j < pm.getNumPresets(); ++j)
                                                   if (pm.getEntry(j).isUser && pm.getEntry(j).name == name) {
                                                       pm.deleteUserPreset(j);
                                                       break;
                                                   }
                                               safeThis->refresh();
                                           }));
    }

    EditHistory& history;
    PresetManager& presets;
    juce::Component* keyTarget = nullptr;

    IconButton btnUndo { IconButton::Icon::Undo }, btnRedo { IconButton::Icon::Redo };
    IconButton btnPrev { IconButton::Icon::Prev }, btnNext { IconButton::Icon::Next };
    IconButton btnSlotA { IconButton::Icon::Text, "A" }, btnSlotB { IconButton::Icon::Text, "B" };
    IconButton btnCopy { IconButton::Icon::Copy };

    juce::Rectangle<int> presetBox, nameArea;
    juce::String shownName;
    bool shownModified = false;
    bool nameHover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkflowBar)
};

} // namespace ff360_ui

#endif
