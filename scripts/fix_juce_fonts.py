import os
import re

def fix_fonts(base_dir):
    pattern_bold_italic = re.compile(r'juce::Font\(\s*juce::FontOptions\(\)\s*\.withHeight\(\s*([0-9\.]+)f?\s*\)\s*\.withStyle\(\s*"Bold Italic"\s*\)\s*\)')
    pattern_bold = re.compile(r'juce::Font\(\s*juce::FontOptions\(\)\s*\.withHeight\(\s*([0-9\.]+)f?\s*\)\s*\.withStyle\(\s*"Bold"\s*\)\s*\)')
    pattern_plain = re.compile(r'juce::Font\(\s*juce::FontOptions\(\)\s*\.withHeight\(\s*([0-9\.]+)f?\s*\)\s*\)')

    dirs_to_check = [
        os.path.join(base_dir, "ff360_ui_core"),
        os.path.join(base_dir, "plugins")
    ]

    count = 0
    for d in dirs_to_check:
        for root, _, files in os.walk(d):
            for f in files:
                if f.endswith(('.h', '.cpp')):
                    path = os.path.join(root, f)
                    with open(path, 'r', encoding='utf-8') as file:
                        content = file.read()

                    new_content = pattern_bold_italic.sub(r'juce::Font(\1f, juce::Font::bold | juce::Font::italic)', content)
                    new_content = pattern_bold.sub(r'juce::Font(\1f, juce::Font::bold)', new_content)
                    new_content = pattern_plain.sub(r'juce::Font(\1f, juce::Font::plain)', new_content)

                    if new_content != content:
                        with open(path, 'w', encoding='utf-8') as file:
                            file.write(new_content)
                        print(f"Updated fonts in: {path}")
                        count += 1

    print(f"Total files updated: {count}")

if __name__ == "__main__":
    base = r"c:\Users\fredd\OneDrive\Desktop\ff360_labs\ff360_labs Synthwave Production Suite\ff360_synthwave_pro_suite\docs"
    fix_fonts(base)
