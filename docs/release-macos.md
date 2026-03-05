# macOS Local Release Pipeline (Signed + Notarized)

This project ships macOS releases fully from your local machine.

## What the local pipeline does

1. Builds `Release` from `Builds/MacOSX/Kronos.xcodeproj` (`Kronos - All`).
2. Signs plugin bundles:
   - `Kronos.vst3`
   - `Kronos.component`
3. Runs the embedded JUCE test suite gate (`./scripts/run_tests_macos.sh Release`) and fails the release if tests fail.
4. Packages both into a signed installer `.pkg`.
   - Default: system plugin paths under `/Library/Audio/Plug-Ins/...`
   - Override option in Installer: "Install for me only" (`~/Library/Audio/Plug-Ins/...`)
   - Format choices are user-selectable in Installer (`VST3`, `AU`) so either can be unchecked.
   - Includes a selectable `User Guide` component rendered from `Kronos_Manual/Kronos Manual/Kronos Manual.md` (canonical source).
5. Notarizes the `.pkg` with Apple `notarytool` and staples the notarization ticket.
6. Writes a SHA-256 checksum file for release distribution.

Scripts:
- `scripts/release/macos-cut-release.sh` (recommended one-command release)
- `scripts/release/macos-release.sh` (low-level build/sign/package/notarize)

## Versioning policy

- Use semver for releases.
- Accepted format examples:
  - `1.0.4`
  - `1.1.0-beta.1`

The script validates this format before building.

## One-time local setup

### 1) Install certificates in your login keychain

Import a `.p12` that contains both certs:

- `Developer ID Application: ...`
- `Developer ID Installer: ...`

You can import via Keychain Access UI, or command line:

```bash
security import /path/to/certs.p12 -k ~/Library/Keychains/login.keychain-db
```

### 2) Store notarization credentials for `notarytool`

```bash
xcrun notarytool store-credentials kronos-plugin \
  --apple-id "<APPLE_ID>" \
  --team-id "<TEAM_ID>" \
  --password "<APP_SPECIFIC_PASSWORD>"
```

## Run a local signed/notarized release (one command)

```bash
./scripts/release/macos-cut-release.sh 1.0.4-beta.1
```

What it does automatically:
- Validates semver version format.
- Updates project/plugin metadata to the core version (`1.0.4` in this example).
- Uses your Developer ID Application + Installer certs.
- Runs build, signing, installer packaging, notarization, stapling, and checksum output.

## Low-level direct run (manual env vars)

```bash
VERSION=1.0.4-beta.1 \
MACOS_APP_SIGNING_IDENTITY="Developer ID Application: Jacob Leone (TEAMID)" \
MACOS_INSTALLER_SIGNING_IDENTITY="Developer ID Installer: Jacob Leone (TEAMID)" \
NOTARIZE=1 \
NOTARY_PROFILE=kronos-plugin \
./scripts/release/macos-release.sh
```

## Output artifacts

Generated in `dist/macos/`:

- `Kronos-<version>.pkg`
- `Kronos-<version>.pkg.sha256`

## Optional local-only modes

- Skip notarization (faster test build): set `NOTARIZE=0`.
- Override installer bundle id: set `PKG_IDENTIFIER=...`.
- Override packaged install locations:
  - `VST3_INSTALL_LOCATION=...`
  - `AU_INSTALL_LOCATION=...`
  - `GUIDE_INSTALL_LOCATION=...`
- Override Xcode scheme/config: set `SCHEME=...` or `CONFIGURATION=...`.
- Prepare version metadata only (no build): set `PREPARE_ONLY=1` on `macos-cut-release.sh`.

## Xcode integration (auto release on `Kronos - All`)

`Builds/MacOSX/Kronos.xcodeproj` now includes an aggregate-target build phase named `Auto Release Notarize Package`.

Behavior:
- Triggers only when `CONFIGURATION=Release`.
- Runs only on scheme `Kronos - All` (because that is where the phase is attached).
- Calls `scripts/release/macos-release.sh` with `SKIP_XCODEBUILD=1` to package already-built artifacts.
- Enforces passing embedded tests before signing/packaging.
- Performs signing, optional notarization, and `.pkg` generation automatically.

Useful environment overrides for Xcode build runs:
- `KRONOS_ENABLE_AUTO_RELEASE=0`: disable auto packaging for that build.
- `KRONOS_RELEASE_VERSION=...`: force version instead of reading `JucePluginDefines.h`.
- `KRONOS_NOTARIZE=0|1`: disable/enable notarization.
- `KRONOS_NOTARY_PROFILE=kronos-plugin`: choose keychain notary profile.
- `KRONOS_APP_SIGN_IDENTITY=...`: override app signing identity.
- `KRONOS_INSTALLER_SIGN_IDENTITY=...`: override installer signing identity.
- `KRONOS_PKG_IDENTIFIER=...`: override installer package identifier.
- `KRONOS_VST3_INSTALL_LOCATION=...`: override VST3 install location inside package.
- `KRONOS_AU_INSTALL_LOCATION=...`: override AU install location inside package.
- `KRONOS_GUIDE_INSTALL_LOCATION=...`: override user guide install location inside package.

## Notes specific to this repo

- Exporter paths currently reference `/Users/jacobleone/Documents/JUCE`.
- Make sure JUCE exists there (or update exporter paths and re-save project files).

## Update log

- 2026-03-01: Added `macos-cut-release.sh` for one-command local signed/notarized installers.
- 2026-03-01: Added rendered/bundled user guide as selectable installer component.
- 2026-03-04: Added script-based release packaging docs and commands for Kronos.
- 2026-03-01: Switched release process to local-only signed/notarized pipeline.
- 2026-03-04: Added auto release packaging build phase on `Kronos - All` (Release only) with env-based overrides.
- 2026-03-04: Added mandatory embedded test gate for release packaging (`run_tests_macos.sh Release`).
- 2026-03-05: Switched packaged user guide source to `Kronos_Manual/Kronos Manual/Kronos Manual.md` and included screenshot assets.

