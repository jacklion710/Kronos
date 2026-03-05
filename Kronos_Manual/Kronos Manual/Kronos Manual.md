## User Manual v1.1.0

<p align="center">
  <img src="./manual_screenshots/Kronos%20Sketch%20Poster.png" alt="Kronos Cover" width="80%" height="80%">
</p>

## Overview

Kronos is a time tracking plugin for your DAW. It tracks how long you work on sessions, stores historical totals, and keeps your session history available between launches.

## New in 1.1.0

- Improved time accounting around day rollover (including midnight, month, and year boundaries).
- Safer session-state restore behavior for older or invalid saved state payloads.
- Stronger same-day session deduplication when repeatedly starting and stopping tracking.
- Release installers now bundle a rendered local user guide generated from this manual.

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
- AU (macOS only)

### System Requirements

- Windows 10 or later
- macOS 10.13 or later
- A DAW that supports VST3 or AU plugins

### Automatic Installers

Kronos installers include selectable plugin format components and bundled user guide files.

## Manual Plugin Paths

### macOS VST3

- Extension: `.vst3`
- System-wide: `/Library/Audio/Plug-Ins/VST3`
- User-only: `~/Library/Audio/Plug-Ins/VST3`

### macOS AU

- Extension: `.component`
- System-wide: `/Library/Audio/Plug-Ins/Components`
- User-only: `~/Library/Audio/Plug-Ins/Components`

### Windows VST3

- Extension: `.vst3`
- Common 64-bit path: `C:\Program Files\Common Files\VST3`

## Kronos UI

<p align="center">
  <img src="./manual_screenshots/Kronos%20Overview.png" alt="Kronos Overview">
</p>

### Main Time Display

<p align="center">
  <img src="./manual_screenshots/Kronos%20Main%20Time%20Display.png" alt="Main Time Display">
</p>

The main display shows total tracked session time in `HH:MM:SS`.

### Controls

#### Play/Pause Button

<p align="center">
  <img src="./manual_screenshots/Kronos%20Tracking%20Button.png" alt="Tracking Button">
</p>

- `Play`: Start tracking time
- `Pause`: Pause tracking time

#### Theme Toggle

<p align="center">
  <img src="./manual_screenshots/Kronos%20Color%20Theme%20Dark.png" alt="Dark Theme">
  <img src="./manual_screenshots/Kronos%20Color%20Theme%20Light.png" alt="Light Theme">
</p>

- Dark mode (default)

<p align="center">
  <img src="./manual_screenshots/Kronos%20Screenshot%20Dark.png" alt="Dark Mode">
</p>

- Light mode

<p align="center">
  <img src="./manual_screenshots/Kronos%20Screenshot%20Light.png" alt="Light Mode">
</p>

#### Sort Button

The sort control to the left of session history supports:

- `Most Recent` (default): newest sessions first
- `Most Time`: highest tracked-time sessions first

### Previous Sessions Panel

<p align="center">
  <img src="./manual_screenshots/Kronos%20Previous%20Sessions.png" alt="Previous Sessions">
</p>

- Shows unlimited previous sessions
- Displays date and duration for each session
- Sorts by recency or tracked duration
- Uses bars to indicate relative session lengths
- Supports list scrolling with up/down arrows

### About Panel

Use the top-right `...` menu and choose `About` to open the about panel with version and credits.

<p align="center">
  <img src="./manual_screenshots/Kronos%20About.png" alt="About Button">
  <img src="./manual_screenshots/Kronos%20About%20Panel.png" alt="About Panel">
</p>

### Data Backup and Restoration

Use the top-right `...` menu:

- `Backup` creates a `.kronos` backup file.
- `Restore` loads a `.kronos` backup file.

Default backup locations:

- macOS: `/Users/<YourUsername>/Library/Application Support/KronosTimeTracker/Backups`
- Windows: `C:\Users\<YourUsername>\AppData\Roaming\KronosTimeTracker\Backups`

## Tips

- A new session bucket is created automatically per day.
- Reopening on the same day keeps accumulating that day's total.
- Pause tracking when taking breaks.
- Save backups regularly.
- Resize the window from corners; aspect ratio is preserved.

## Troubleshooting

### Plugin Not Appearing

- Confirm your DAW scans the correct plugin folder.
- Restart or rescan plugins in your DAW.

### Plugin Not Saving Time

- Confirm your DAW allows plugin state persistence.
- Confirm write permissions for your user data directories.

### macOS Security Prompt

- If macOS blocks loading, review Security settings and allow the plugin if needed.

## Support

- Website: https://jacklion.xyz
- Email: jack.lion710@gmail.com
- Instagram: [@jack.lion](https://www.instagram.com/jack.lion)
- Discord: [Join my community](https://discord.gg/EFQq7BX)
- Gumroad: [More plugins](https://jacklion.gumroad.com)

## Credits

- Created by Jacob Leone (Jack.Lion)
- Graphics by [Aznadel](https://linktr.ee/aznadel)

---

Note: This manual reflects version 1.1.0 of Kronos.
