# Kronos - DAW Time Tracking Plugin

![Kronos Plugin](Resources/screenshot.png)

Kronos is a simple yet powerful time tracking plugin for your DAW. Keep track of how much time you spend on your music production sessions with an elegant, easy-to-use interface.

## Features

- Real-time session tracking
- Unlimited session history
- Dark/Light theme support
- Session sorting by date or duration
- Persistent data storage
- Resizable interface
- Visual time bars for session comparison

## Installation

### Supported Formats
- VST3
- AU (Mac only)
- Standalone

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

2. Download and install JUCE v7.0.12 from [JUCE's website](https://juce.com/get-juce/download)

Note: You may need to visit the JUCE github [releases](https://github.com/juce-framework/JUCE/releases) page to find the correct version.

3. Open `Kronos.jucer` in Projucer

4. Generate project files for your IDE

5. Build the project in your IDE

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

## Credits

Created by Jacob Leone (Jack.Lion)
Graphics by Aznadel

## Links

- Website: https://jacklion.com
- Instagram: @jack.lion
- Discord: [\[Join our community\]](https://discord.gg/EFQq7BX)
- Gumroad: [\[More plugins\]](https://jacklion.gumroad.com)

## License

...