# Windows VS2022 Release Pipeline (Local-First)

This project supports a local Windows release packaging flow for:

- Standalone app (`Kronos.exe`)
- VST3 plugin bundle (`Kronos.vst3`)

AU is not built on Windows.

## Current trust model (beta)

Windows code-signing is intentionally disabled for now. Installers and binaries are unsigned, so users may see SmartScreen/Unknown Publisher warnings and choose to continue.

## Prerequisites (Windows machine)

- Visual Studio 2022 with C++ workload
- Inno Setup 5 or 6 (`ISCC.exe`)
  - default lookup paths:
    - `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`
    - `C:\Program Files\Inno Setup 6\ISCC.exe`
    - `C:\Program Files (x86)\Inno Setup 5\ISCC.exe`
    - `C:\Program Files\Inno Setup 5\ISCC.exe`
  - or set `KRONOS_INNO_COMPILER` to custom path

## IDE auto-packaging behavior

Release post-build hooks are wired in:

- `Builds/VisualStudio2022/Kronos_StandalonePlugin.vcxproj`
- `Builds/VisualStudio2022/Kronos_VST3.vcxproj`

Hook command:

```bat
if /I "$(ProjectName)"=="Kronos_StandalonePlugin" powershell -NoProfile -ExecutionPolicy Bypass -File "$(SolutionDir)..\..\scripts\release\windows-release.ps1" -SolutionDir "$(SolutionDir)..\.." -Configuration "$(Configuration)" -Platform "$(Platform)" -TriggeredByPostBuild
```

This gate ensures packaging runs once per solution build (from Standalone project only).

## Manual command (optional)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release/windows-release.ps1 `
  -SolutionDir "." -Configuration "Release" -Platform "x64"
```

## One-command release (recommended)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release/windows-cut-release.ps1 `
  -Version "1.1.0-beta.1"
```

This wrapper:

- validates version format (`x.y.z` or `x.y.z-beta.N`)
- updates project/plugin version metadata to core semver (`1.1.0` in this example)
- builds `Kronos - VST3` and `Kronos - Standalone Plugin` in `Release|x64`
- runs the embedded JUCE test gate (`scripts/run_tests_windows.ps1 -Configuration Release`) using the same in-plugin test suite as macOS
- runs installer packaging and writes checksums

## Output

- `dist/windows/Kronos-<version>-setup.exe`
- `dist/windows/Kronos-<version>-setup.exe.sha256`

## Installer contents

Installer provides selectable components:

- Standalone application
- VST3 plugin
- User Guide files (rendered from `Kronos_Manual/Kronos Manual/Kronos Manual.md` as canonical source, including `manual_screenshots/` assets)

Installer shows separate destination pages:

- Standalone/User Guide default: `C:\Program Files\Kronos`
- VST3 default: `C:\Program Files\Common Files\VST3`

## Optional environment controls

- `KRONOS_WINDOWS_AUTO_RELEASE=0` disables auto-packaging from build hooks.
- `KRONOS_INNO_COMPILER=<full path to ISCC.exe>` overrides Inno compiler path.

## Manual test command (Windows)

Run the Windows embedded test suite directly:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_tests_windows.ps1 -Configuration Release
```

## Update log

- 2026-03-01: Added VS2022 Release auto-packaging pipeline for Standalone + VST3.
- 2026-03-01: Added `windows-cut-release.ps1` one-command Windows release wrapper.
- 2026-03-05: Added mandatory Windows release test gate and fail-on-test-failure behavior.
- 2026-03-05: Added rendered/bundled user guide assets from `Kronos_Manual/Kronos Manual/Kronos Manual.md`.
