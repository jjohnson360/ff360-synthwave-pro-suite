# Fix: macOS VST3/AU Release Packaging — Antigravity Prompt

**Symptom:** the GitHub release's macOS-labeled artifact folder contains Windows VST3 binaries instead of real macOS `.vst3`/`.component` bundles.

**Goal:** produce a genuine macOS 12+ beta build — proper `.vst3` and `.component` (AU) bundles, correctly packaged — and publish it as a **brand-new GitHub release**, not an edit to an existing one.

---

## 0. Diagnose before touching packaging

This bug pattern (Windows binaries showing up under a "macOS" folder) almost always comes from one of these — check each and report which one it actually is before fixing anything:

1. **Single-runner build:** the workflow only runs on `windows-latest` and the "macOS" folder is just a renamed copy of the Windows output, not a real cross-platform matrix build.
2. **Matrix build exists but the macOS job is silently failing** (e.g. missing Xcode, wrong CMake generator, missing `CMAKE_OSX_*` flags) and a later step falls back to copying the Windows artifact into the macOS path so the workflow doesn't show a hard failure.
3. **Artifact upload/download step is misrouting paths** — both jobs run correctly, but `actions/upload-artifact` / `download-artifact` steps use the same artifact name or a hardcoded path that isn't OS-namespaced, so the macOS job's output gets overwritten by the Windows job's output (or vice versa) before the release step runs.
4. **Windows `.vst3` is a single file; macOS `.vst3` is a bundle (directory).** If the packaging step zips things generically without checking bundle structure, a flat-file Windows `.vst3` can get dropped into a folder that's *labeled* macOS without anyone noticing it's the wrong file type, since both have the same extension.

Print the actual workflow YAML and the job logs from the most recent failed/incorrect run before making changes, so the fix targets the real cause rather than guessing.

---

## 1. Correct macOS build job

The macOS job must run on a macOS GitHub-hosted runner (`macos-13` or `macos-14`) and build natively — never cross-compile macOS binaries from a Windows or Linux runner.

**Required CMake configuration for the macOS job specifically:**

```cmake
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
```

- `CMAKE_OSX_DEPLOYMENT_TARGET` must be `12.0`, matching the "macOS 12+" requirement — confirm this isn't left at the Xcode default (which can be much newer and silently exclude older systems) or unset entirely.
- Build a **universal binary** (arm64 + x86_64) unless there's a specific reason to ship Apple-silicon-only. After building, verify with:
  ```bash
  lipo -info path/to/Plugin.vst3/Contents/MacOS/Plugin
  ```
  Expected output lists both `x86_64` and `arm64`.
- Use Xcode generator or Ninja on the macOS runner — do not reuse the same CMake generator/toolchain config across both OS jobs. If the workflow currently shares one `cmake -B build ...` step across a matrix without OS-specific flags, that's very likely the root cause from Step 0.

**JUCE target formats for the macOS job:**
```cmake
juce_add_plugin(YourPlugin
    FORMATS VST3 AU
    ...
)
```
Do not include `Standalone` unless it's actually wanted — keep the macOS job's outputs to exactly what's meant to ship.

---

## 2. Confirm real bundle structure, not flat files

Both `.vst3` and `.component` are **directories** on macOS (bundles), not single files. After the build, verify structure explicitly as a CI step, not just visually:

```bash
find build/ -name "*.vst3" -o -name "*.component" | while read bundle; do
  echo "Checking: $bundle"
  test -d "$bundle" || { echo "FAIL: not a directory bundle"; exit 1; }
  test -f "$bundle/Contents/Info.plist" || { echo "FAIL: missing Info.plist"; exit 1; }
  file "$bundle/Contents/MacOS/"* | grep -q "Mach-O" || { echo "FAIL: not a Mach-O binary"; exit 1; }
done
```

If this check ever fails, stop the workflow there instead of packaging and releasing a broken artifact — that's the exact bug being fixed, so don't let it ship silently again.

---

## 3. Signing (beta-appropriate, not full notarization)

Full Apple notarization requires a paid Developer ID and is likely out of scope for a beta. At minimum, ad-hoc sign so Gatekeeper doesn't flatly refuse to open the bundle:

```bash
codesign --force --deep --sign - "path/to/Plugin.vst3"
codesign --force --deep --sign - "path/to/Plugin.component"
```

Note in the release notes that this is an ad-hoc-signed beta build and users may need to right-click → Open (or clear the quarantine attribute with `xattr -cr`) the first time, since it isn't notarized.

---

## 4. Package correctly

Zip the actual bundle directories with their structure intact — don't let a generic "zip the build output folder" step flatten or partially copy bundle internals.

```bash
cd build/macOS
zip -r ff360_labs_<PluginName>_macOS.zip *.vst3 *.component
```

Validate the zip round-trips correctly before uploading — unzip it in a clean temp directory and re-run the Step 2 structure check against the unzipped copy, not just the pre-zip build output. A packaging bug can corrupt structure even when the pre-zip build was correct.

---

## 5. AU validation (macOS-specific, don't skip)

Run Apple's `auval` against the built AU component as a CI gate:

```bash
auval -v aufx <SubType> <Manufacturer>
```
(substitute the actual 4-char codes from your `juce_add_plugin` config). A failing `auval` should fail the workflow, not just log a warning — an AU that fails validation won't load in Logic/GarageBand/MainStage even if the file exists.

---

## 6. Create a genuinely new release — do not edit an existing one

This is the second half of the bug report: prior runs have been *modifying* an existing GitHub release instead of creating a new one. Fix the release step explicitly:

- **Tag:** generate a new, unused tag for every release run — e.g. `v0.x.y-beta.N` with `N` incrementing, or derive from the build date/commit if there's no formal version scheme yet. Never reuse an existing tag name; if the workflow currently computes the tag from a static value in a config file that isn't bumped automatically, that's why it keeps overwriting the same release.
- **Release creation:** use `gh release create <new-tag> ...` (not `gh release upload` / `gh release edit`, which target an *existing* release) — or if using `actions/create-release` / `softprops/action-gh-release`, confirm the action is invoked with a fresh `tag_name` and not `overwrite: true` / a fixed release ID.
- **Verify before running for real:** dry-run or `--dry-run`-equivalent check (or just print the computed tag name as a workflow step) so you can confirm it's actually new before the release fires, given the history of this silently overwriting things.
- **Release assets:** attach the macOS zip from Step 4 and the Windows equivalent as separate, clearly named assets on the same new release (e.g. `ff360_labs_<PluginName>_macOS.zip`, `ff360_labs_<PluginName>_Windows.zip`) — don't let one OS's upload step overwrite the other's asset slot.

---

## 7. Acceptance criteria for this fix

- [ ] Workflow logs show two genuinely separate OS jobs running, each on its correct runner.
- [ ] macOS job's `lipo -info` output confirms a universal (arm64 + x86_64) binary.
- [ ] macOS job's `CMAKE_OSX_DEPLOYMENT_TARGET` is confirmed as `12.0` in the build log.
- [ ] Structure check (Step 2) passes on both the pre-zip build output and the re-unzipped copy.
- [ ] `auval` passes for the AU component.
- [ ] The zip downloaded from the actual GitHub release page (not the CI artifact) contains real Mach-O bundles when inspected on a Mac — this is the final check, since CI artifacts and release assets have gone through different upload paths before.
- [ ] The release created is a new tag/release, visibly separate from prior releases in the repo's release list, with its own release notes — not an edit to a prior release's assets or notes.
