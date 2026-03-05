## User Manual v1.0.4

<p align="center">
  <img src="./manual_screenshots/Kronos%20Sketch%20Poster.png" alt="Kronos Cover" width="80%" height="80%">
</p>

## Overview

Kronos is a simple yet powerful time tracking plugin for your DAW. It helps you monitor how much time you spend on your music production sessions, with features to track daily sessions and view historical data.

## Features

- Real-time session tracking
- Unlimited daily session history
- Dark/Light theme support
- Session sorting by date or duration
- Persistent data storage
- Data backup and restoration
- Resizable interface

## Installation

### Supported Formats

- VST3
- AU (Mac only)

### System Requirements

- Windows 10 or later
- macOS 10.13 or later
- A DAW that supports VST3 or AU plugins

### Guide

Below is a short guide on how to manually install your plugin files onto macOS and Windows systems.

## macOS

### VST3 Plugins

• Plugin file extension: .vst3

• Default location:

`Library/Audio/Plug-Ins/VST3` (system-wide)

or

`~/Library/Audio/Plug-Ins/VST3` (user-only)

### AU Plugins

• Plugin file extension: .component

• Default location:

`/Library/Audio/Plug-Ins/Components` (system-wide)

or

`~/Library/Audio/Plug-Ins/Components` (user-only)

### Automatic Installation

To install automatically:

- Close your DAW.
- Locate your `Kronos.pkg` file.
- Double click to run the installer.
- Relaunch your DAW, and it should detect the plugin.

If you wish to have it installed elsewhere you should see an option in the package installation window that allows you to choose a custom path. If not, simply move the files from their default installation location into the desired path.

---

## Windows

### VST3 Plugins

• Plugin file extension: `.vst3`

• Common default location:

`C:\Program Files\Common Files\VST3` (64-bit)

To install Automatically:

- Close your DAW.
- Locate your `KronosInstaller.exe` file.
- Double click to run the installer.
- Relaunch your DAW, and it should detect the plugin.

▶ Note: Some DAWs let you configure custom plugin folders. If yours looks like it installed in a different location, place your `.vst3` in that folder instead or add an additional scan path in your DAW settings.

---

## Troubleshooting

• After moving the plugin, always relaunch your DAW to ensure it scans the new or updated plugin file.

• On Windows, if your plugin doesn't appear, confirm that your DAW is scanning the correct folder in its preferences/settings.

• On macOS, if the plugin still fails to load, you may need to open Security & Privacy settings to grant permission to load software from an "unidentified developer".

With these steps, you should be able to automatically install both VST3 and AU plugins on macOS, and VST3 plugins on Windows, ensuring they're detected and loaded properly in your DAW.

## Kronos UI

<p align="center">
  <img src="./manual_screenshots/Kronos%20Overview.png" alt="Kronos Overview">
</p>

### Main Time Display

<p align="center">
  <img src="./manual_screenshots/Kronos%20Main%20Time%20Display.png" alt="Main Time Display">
</p>

The main time display shows your current session time in hours, minutes, and seconds format (HH:MM:SS).

### Controls

#### Play/Pause Button

<p align="center">
  <img src="./manual_screenshots/Kronos%20Tracking%20Button.png" alt="Tracking Button">
</p>

Located beneath the time display, this button controls time tracking:

- ▶️ Play: Start tracking time
- ⏸️ Pause: Pause time tracking

#### Theme Toggle

<p align="center">
  <img src="./manual_screenshots/Kronos%20Color%20Theme%20Dark.png" alt="Dark Theme">
  <img src="./manual_screenshots/Kronos%20Color%20Theme%20Light.png" alt="Light Theme">
</p>

Located in the bottom-right corner:

- 🌙 Dark mode (default)

<p align="center">
  <img src="./manual_screenshots/Kronos%20Screenshot%20Dark.png" alt="Dark Mode">
</p>

- ☀️ Light mode

<p align="center">
  <img src="./manual_screenshots/Kronos%20Screenshot%20Light.png" alt="Light Mode">
</p>

#### Sort Button

Located to the left of the session history:

- Sort by Most Recent (default): Shows most recent sessions at the top and descends by date
- Sort by Most Time: Shows

### Previous Sessions Panel

<p align="center">
  <img src="./manual_screenshots/Kronos%20Previous%20Sessions.png" alt="Previous Sessions">
</p>

- Shows unlimited previous sessions
- Displays date and duration for each session
- Sort dates by recency or by time spent
- Visual bars indicate relative session lengths
- Use up/down arrows to scroll through sessions

### About Panel

Located in the top right corner of the UI `...`, clicking shows a dropdown menu. Selecting `About` brings up a modal display including the current version as well as credits.

<p align="center">
  <img src="./manual_screenshots/Kronos%20About.png" alt="About Button">
  <img src="./manual_screenshots/Kronos%20About%20Panel.png" alt="About Panel">
</p>

### Data Backup & Restoration

Located in the top right corner of the UI `...`, clicking shows a dropdown menu. Selecting `Backup` brings up a modal display which allows users to save a backup of their time data in a `.kronos` log file. Selecting `Restore` allows users to select a log file to restore from.

**Note:** If you wish to delete a backup log, they may be overwritten or otherwise deleted manually via the MacOS Finder or Windows File Explorer.

- **MacOS path:** `/Users/<YourUsername>/Library/Application Support/KronosTimeTracker/Backups`
- **Windows path:** `C:\Users\<YourUsername>\AppData\Roaming`

## Tips & Tricks

- Each day automatically creates a new session
- All previous sessions are retained
- Press pause when taking a break from the session and press play when resuming
- Time accumulates per day when you reopen a project on the same date
- Users should save backup logs for restoration often in case of unexpected bugs
- Interface Scaling:

  - Grab any corner to resize the plugin window
  - Maintains aspect ratio for consistent appearance

## Troubleshooting

### Common Issues

Plugin not saving time:

- Ensure your DAW has proper plugin state saving enabled
- Check if you have write permissions in your DAW's plugin data directory

Time tracking seems inaccurate:

- Time tracking may pause when your DAW is not the active window
- Some DAWs may suspend plugin processing when audio is stopped

## Support & Feedback

### Bug Reports & Feature Requests

Visit our Discord server for support and to submit feature requests or bug reports.

### Links

- Website: https://jacklion.xyz
- Email: jacklion710@gmail.com
- Instagram: [@jack.lion](https://www.instagram.com/jack.lion)
- Discord: [Join my community](https://discord.gg/EFQq7BX)
- Gumroad: [More plugins](https://jacklion.gumroad.com)

## Credits

- Created by Jacob Leone (Jack.Lion)
- Graphics by [Aznadel](https://linktr.ee/aznadel)

---

Note: This manual reflects version 1.0.2 of Kronos. Features and interface may change in future updates.
