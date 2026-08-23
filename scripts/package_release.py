import os
import zipfile

def create_releases():
    base_dir = r"c:\Users\fredd\OneDrive\Desktop\ff360_labs\ff360_labs Synthwave Production Suite\ff360_synthwave_pro_suite\docs"
    dist_dir = os.path.join(base_dir, "dist")
    os.makedirs(dist_dir, exist_ok=True)

    win_zip_path = os.path.join(dist_dir, "ff360_Synthwave_FX_Suite_v1.0.0-beta_Windows_x64.zip")
    mac_zip_path = os.path.join(dist_dir, "ff360_Synthwave_FX_Suite_v1.0.0-beta_macOS_Universal.zip")

    # Files common to both
    common_files = [
        "README.md",
        "manuals/ff360_Synthwave_Production_Suite_User_Manual.pdf",
        "CMakeLists.txt"
    ]

    common_dirs = [
        "ff360_dsp_core/include",
        "ff360_ui_core/include",
        "plugins",
        "Walkthroughs",
        "implementation_plans"
    ]

    # 1. Build Windows ZIP
    with zipfile.ZipFile(win_zip_path, 'w', zipfile.ZIP_DEFLATED) as win_zip:
        for rel_file in common_files:
            abs_p = os.path.join(base_dir, rel_file)
            if os.path.exists(abs_p):
                win_zip.write(abs_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta", rel_file))

        for rel_dir in common_dirs:
            abs_dir = os.path.join(base_dir, rel_dir)
            for root, _, files in os.walk(abs_dir):
                for f in files:
                    full_p = os.path.join(root, f)
                    rel_p = os.path.relpath(full_p, base_dir)
                    win_zip.write(full_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta", rel_p))

        # Add compiled Windows binaries
        bin_files = [
            ("build/Release/ff360_dsp_core.lib", "bin/ff360_dsp_core.lib"),
            ("build/Release/ff360_suite_benchmarks_collection.exe", "bin/ff360_suite_benchmarks_collection.exe"),
            ("build/Release/ff360_dsp_core_tests.exe", "bin/ff360_dsp_core_tests.exe")
        ]
        for src_rel, dst_rel in bin_files:
            abs_p = os.path.join(base_dir, src_rel)
            if os.path.exists(abs_p):
                win_zip.write(abs_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta", dst_rel))

        # Add compiled VST3 bundles
        vst_dir = "C:/Program Files/Common Files/VST3/ff360 Labs"
        if os.path.exists(vst_dir):
            for root, _, files in os.walk(vst_dir):
                for f in files:
                    full_p = os.path.join(root, f)
                    rel_p = os.path.relpath(full_p, vst_dir)
                    win_zip.write(full_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta/VST3", rel_p))

    print(f"Windows release package created at: {win_zip_path}")

    # 2. Build macOS Universal ZIP
    with zipfile.ZipFile(mac_zip_path, 'w', zipfile.ZIP_DEFLATED) as mac_zip:
        for rel_file in common_files:
            abs_p = os.path.join(base_dir, rel_file)
            if os.path.exists(abs_p):
                mac_zip.write(abs_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta", rel_file))

        full_source_dirs = [
            "ff360_dsp_core",
            "ff360_ui_core",
            "plugins",
            "Walkthroughs",
            "implementation_plans"
        ]
        for rel_dir in full_source_dirs:
            abs_dir = os.path.join(base_dir, rel_dir)
            for root, _, files in os.walk(abs_dir):
                for f in files:
                    if f.endswith(('.obj', '.lib', '.exe', '.ilk', '.pdb', '.vcxproj', '.filters')):
                        continue
                    full_p = os.path.join(root, f)
                    rel_p = os.path.relpath(full_p, base_dir)
                    mac_zip.write(full_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta", rel_p))

        # Add macOS binaries (VST3/AU) from release_assets
        mac_bin_dir = "release_assets"
        abs_bin_dir = os.path.join(base_dir, mac_bin_dir)
        if os.path.exists(abs_bin_dir):
            for root, _, files in os.walk(abs_bin_dir):
                for f in files:
                    full_p = os.path.join(root, f)
                    rel_p = os.path.relpath(full_p, abs_bin_dir)
                    mac_zip.write(full_p, arcname=os.path.join("ff360_Synthwave_Suite_v1.0.0-beta/macOS", rel_p))

    print(f"macOS Universal package created at: {mac_zip_path}")

if __name__ == "__main__":
    create_releases()
