## User Manual v1.0.0-Beta.2

## Overview

Kronos is a simple yet powerful time tracking plugin for your DAW. It helps you monitor how much time you spend on your music production sessions, with features to track daily sessions and view historical data.

## Features

- Real-time session tracking

- Unlimited daily session history

- Dark/Light theme support

- Session sorting by date or duration

- Persistent data storage

- Resizable interface

## Installation

### Supported Formats

- VST3

- AU (Mac only)

### System Requirements

- Windows 10 or later

- macOS 10.13 or later

- A DAW that supports VST3 or AU plugins

### Guide

Below is a short guide on how to manually install your plugin files onto macOS and Windows systems. 

## macOS

### VST3 Plugins

• Plugin file extension: .vst3

• Default location:

`Library/Audio/Plug-Ins/VST3` (system-wide)

or

`~/Library/Audio/Plug-Ins/VST3` (user-only)

To install manually:

- Close your DAW.

- Locate your `.vst3` file.

- Copy or move the `.vst3` file to one of the folders listed above.

- Relaunch your DAW, and it should detect the plugin.

### AU Plugins

• Plugin file extension: .component

• Default location:

`/Library/Audio/Plug-Ins/Components` (system-wide)

or

`~/Library/Audio/Plug-Ins/Components` (user-only)

To install manually:

- Close your DAW (e.g., Logic, Bitwig, etc.).

- Copy or move the `.component` file to one of the folders listed above.

- Relaunch your DAW, and it should detect the plugin automatically.

▶ Note: On macOS 10.15 (Catalina) or later, you may need to grant permissions, remove the “quarantine” attribute, or sign/notarize the plugin in order for the OS to recognize and load it properly. If you see errors about “unidentified developer,” open an instance of terminal and run:

`xattr -cr "/path/to/MyPlugin.component"`

or

`xattr -cr "/path/to/MyPlugin.vst3"`

---

## Windows

### VST3 Plugins

• Plugin file extension: `.vst3`

• Common default location:

`C:\Program Files\Common Files\VST3` (64-bit)

To install manually:

- Close your DAW.

- Locate your `.vst3` folder

- Copy or move the `.vst3` folder into one of the `Common Files\VST3` folders.

- Relaunch your DAW, and it should detect the plugin.

▶ Note: Some DAWs let you configure custom plugin folders. If yours looks in a different location, place your `.vst3` in that folder instead or add an additional scan path in your DAW settings.

---

## Troubleshooting

• After moving the plugin, always relaunch your DAW to ensure it scans the new or updated plugin file.

• On Windows, if your plugin doesn’t appear, confirm that your DAW is scanning the correct folder in its preferences/settings.

• On macOS, if the plugin still fails to load, you may need to open Security & Privacy settings to grant permission to load software from an “unidentified developer,” or remove “quarantine” attributes, as mentioned above.

With these steps, you should be able to manually install both VST3 and AU plugins on macOS, and VST3 plugins on Windows, ensuring they’re detected and loaded properly in your DAW.

.gitignore
## Interface Guide

### Main Display

The main display shows your current session time in hours, minutes, and seconds format (HH:MM:SS).

### Controls

#### Play/Pause Button

Located beneath the time display, this button controls time tracking:

- ▶️ Play: Start tracking time

- ⏸️ Pause: Pause time tracking

#### Theme Toggle

Located in the top-right corner:

- 🌙 Dark mode (default)

- ☀️ Light mode

#### Sort Button

Located to the left of the session history:

- Sort by Most Recent (default)

- Sort by Most Time

### Previous Sessions Panel

- Shows unlimited previous sessions

- Displays date and duration for each session

- Visual bars indicate relative session lengths

- Use up/down arrows to scroll through sessions

## Tips & Tricks

- Each day automatically creates a new session

- All previous sessions are retained

- Press pause when taking a break from the session and press play when resuming

- Time accumulates per day when you reopen a project on the same date

- Interface Scaling:

	- Grab any corner to resize the plugin window
	
	- Maintains aspect ratio for consistent appearance

## Troubleshooting

### Common Issues

Plugin not saving time:

- Ensure your DAW has proper plugin state saving enabled

- Check if you have write permissions in your DAW's plugin data directory

Time tracking seems inaccurate:

- Time tracking may pause when your DAW is not the active window

- Some DAWs may suspend plugin processing when audio is stopped

## Support & Feedback

### Bug Reports & Feature Requests

Visit our Discord server for support and to submit feature requests or bug reports.

### Links

- Website: https://jacklion.xyz

- Email: jacklion710@gmail.com

- Instagram: [@jack.lion](https://www.instagram.com/jack.lion)

- Discord: [Join my community](https://discord.gg/EFQq7BX)

- Gumroad: [More plugins](https://jacklion.gumroad.com)

## Credits

- Created by Jacob Leone (Jack.Lion)

- Graphics by Aznadel

---

Note: This manual reflects version 1.0.0-Beta.2 of Kronos. Features and interface may change in future updates.