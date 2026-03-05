# Kronos - DAW Time Tracking Plugin

V1.0.3

**Official release coming March 1st 2025!**

<p align="center">
  <img src="./Kronos_Manual/Kronos%20Manual/manual_screenshots/Kronos%20Screenshot%20Dark.png" alt="Kronos UI">
</p>

Kronos is a simple yet powerful time tracking plugin for your DAW. Keep track of how much time you spend on your music production sessions with an elegant, easy-to-use interface.

## Features

- Real-time session tracking
- Unlimited session history
- Dark/Light theme support
- Session sorting by date or duration
- Persistent data storage
- Data backup and restoration
- Resizable interface
- Visual time bars for session comparison

## Installation

### Supported Formats

- VST3
- AU (Mac only)

### System Requirements

- Windows 10 or later
- macOS 10.13 or later
- A DAW that supports VST3 or AU plugins

## Building from Source

### Prerequisites

- JUCE Framework v7.0.12
- C++20 compatible compiler
- Visual Studio 2022 (Windows) or Xcode (macOS)

### Build Steps

1. Clone the repository:

```bash
git clone https://github.com/jacklion710/kronos.git
```

2. Download and install JUCE v7.0.12 from [JUCE&#39;s website](https://juce.com/get-juce/download)

**Note:** You may need to visit the JUCE github [releases](https://github.com/juce-framework/JUCE/releases) page to find the correct version.

3. Open `Kronos.jucer` in Projucer
4. Generate project files for your IDE
5. Build the project in your IDE

## Testing

Kronos now has an embedded JUCE unit test suite for processor/state logic.

### Test Mode

Use these environment variables when launching the standalone target:

- `KRONOS_TEST_MODE=1`: disables auto-start tracking so tests are deterministic.
- `KRONOS_RUN_TESTS=1`: runs the embedded test suite on launch and quits.
- `KRONOS_TEST_RESULTS_FILE=/path/to/file`: writes total failing assertions to the file.

### One-Command Local Test Run (macOS)

From the repo root:

```bash
./scripts/run_tests_macos.sh
```

Optional configuration:

```bash
./scripts/run_tests_macos.sh Release
```

### What the tests currently cover

- State serialization debounce behavior
- New-day session insertion when tracking starts
- Midnight rollover behavior across multiple processor instances
- Debounced state byte reuse (rapid serialization safety)
- Stop-tracking elapsed-time accumulation
- Timer callback per-day and total counter increments
- Legacy/older state restore inserts current day
- Date sorting correctness for `MostTime` mode

## Project Structure

- `/Source` - Core plugin implementation
- `/Resources` - Assets (SVGs, fonts, etc.)
- `/JuceLibraryCode` - JUCE framework code

## Development

The plugin is built using JUCE framework and follows a standard audio plugin architecture:

- `PluginProcessor` - Handles core functionality and state management
- `PluginEditor` - Manages the UI and user interactions
- `LookAndFeel` - Custom styling and theme implementation
- `AboutComponent` - About window with links and information
- `BackupComponent` - Backup window for saving `.kronos` log files with project time data
- `RestoreComponent` - Restore window for loading `.kronos` log files into the project time

## Release Pipeline

Kronos now includes the same local-first Mac/Windows release pipeline structure used in your other JUCE plugin repos.

- macOS docs: `docs/release-macos.md`
- Windows docs: `docs/release-windows.md`
- Release scripts: `scripts/release/`
- macOS `Kronos - All` + `Release` now auto-runs signing/notarization/pkg packaging (override with env vars documented in `docs/release-macos.md`)

Common commands:

```bash
./scripts/release/macos-cut-release.sh 1.0.4-beta.1
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release/windows-cut-release.ps1 -Version "1.0.4-beta.1"
```

## Credits

Created by Jacob Leone (Jack.Lion)
Graphics by Aznadel

## Links

- Website: [\[Jack.Lion Official Website\]](https://jacklion.xyz)
- Instagram: [\[@jack.lion\]](https://www.instagram.com/jack.lion)
- Discord: [\[Join my community\]](https://discord.gg/EFQq7BX)
- Gumroad: [\[More plugins\]](https://jacklion.gumroad.com)
- Tech Portfolio: [\[Jacob Leones Tech Portfolio\]](https://jacobleone.tech)

### Contributing Code

- Fork the repository
- Create a feature branch (`git checkout -b feature/NewFeature`)
- Commit your changes (`git commit -m 'Add some NewFeature'`)
- Push to the branch (`git push origin feature/NewFeature`)
- Open a Pull Request

#### Guidelines

- Follow existing code style and conventions
- Update documentation as needed at `./Kronos_Manual/Kronos Manual/Kronos Manual.md`

#### Feature Requests & Bug Reports

- Use the Issues tab on GitHub to submit feature requests or bug reports (**preffered**)
- Clearly describe the proposed feature or bug with relevant details
- For bugs, include steps to reproduce and expected behavior
- For bug reports you may also reach me directly at [jack.lion710@gmail.com](mailto:jack.lion710@gmail.com)

#### Getting Help

- Check existing Issues and Pull Requests before creating new ones
- Join my Discord community for deeper discussions and questions
- Review the documentation in the Kronos Manual

I appreciate all contributions that help make Kronos better!

## License

Kronos is licensed under the GPL v3. Redistribution of compiled binaries is discouraged unless significant modifications are made. See the LICENSE file for details.
