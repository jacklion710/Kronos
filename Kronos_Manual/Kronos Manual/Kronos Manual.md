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
- Data backup and restoration with a compare step before replacing
- Today, this run, daily average and longest day stats
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
  <img src="./manual_screenshots/Kronos%20v2%20Dark.png" alt="Kronos in the dark theme, tracking" width="80%">
</p>

The window has a header, a readout panel and a sessions panel. It scales as one piece from 400 x 300 to 1200 x 900, keeping its 4:3 shape.

### Header

- **Status chip**: shows `TRACKING` or `PAUSED`.
- **Theme toggle**: switches between the dark (default) and light themes.
- **Menu**: opens Back up, Restore and About.

### Readout Panel

- **Dial**: the outer arc shows the position in the current minute and the inner arc the position in the current hour, both from your real project time. The ring of planetary symbols and the hexagram are decorative. They turn only while tracking and stop when paused.
- **Start/Pause**: the button in the centre of the dial.
- **Project time**: everything tracked in this project, as hours, minutes and seconds.
- **This run**: time since tracking last started.
- **Today**: time tracked on the current calendar day.
- **Daily avg**: project time divided by the number of days that have tracked time.
- **Longest day**: the most time tracked on a single day.

### Sessions Panel

- Lists every day with tracked time, and scrolls with your trackpad, mouse wheel or the scrollbar.
- `RECENT` sorts newest first. `LONGEST` sorts by tracked time.
- Each row shows the weekday, date, a bar scaled to your longest day, and the duration.
- Today is marked `TDY` in the accent colour, and your longest day has the brightest bar.

<p align="center">
  <img src="./manual_screenshots/Kronos%20v2%20Light%20Menu.png" alt="Kronos in the light theme with the menu open" width="80%">
</p>

### Data Backup and Restoration

Open the menu in the top right corner.

**Back up** saves this project's full time log as a `.kronos` file. Enter a project name and check the file name shown beneath it. If that name already exists, Kronos asks before overwriting it. The saved screen only appears once the file has really been written. If saving fails, Kronos says why instead.

**Restore** lists your saved backups with their totals. Choose one, or use `Browse...` to pick a file elsewhere, then compare your current log with the backup before deciding. `Keep current` is selected by default, and `Replace log` overwrites this project's log.

Default backup locations:

- macOS: `/Users/<YourUsername>/Library/Application Support/KronosTimeTracker/Backups`
- Windows: `C:\Users\<YourUsername>\AppData\Roaming\KronosTimeTracker\Backups`

### About

Open the menu and choose `About Kronos` for the version, credits and links.

## Tips

- A new session bucket is created automatically per day.
- Reopening on the same day keeps accumulating that day's total.
- Time while your computer is asleep is not counted.
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

- Website: https://jacklionaudio.com
- Email: jack.lion710@gmail.com
- Instagram: [@jack.lion](https://www.instagram.com/jack.lion)
- Gumroad: [More plugins](https://jacklion.gumroad.com)

## Credits

- Created by Jacob Leone (Jack.Lion)
- V1 design by [Aznadel](https://linktr.ee/aznadel)

---

Note: This manual reflects version 1.1.0 of Kronos.
