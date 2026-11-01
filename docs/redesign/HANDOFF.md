# Kronos v2 UI redesign — implementation handoff

You are implementing an approved UI redesign of **Kronos**, a JUCE (v7.0.12, C++20) DAW time-tracking plugin (VST3/AU). Repo root: this repository. Work on a new branch `feat/ui-redesign-v2`. Do not push, tag, cut a release or run the signing/notarization scripts unless asked.

## Source of truth

The design lives in `docs/redesign/` (copied from the Kronos Redesign design canvas):

- `Main.dc.html` — the full plugin window at its 600×450 base size, as an interactive HTML prototype. **Read its markup and the `KR_THEMES` object for exact colors, sizes, positions and copy.** Its `renderVals()` shows the state logic (tracking, theme, sort, menu, sheets).
- `Paused.dc.html`, `Light.dc.html`, `Backup.dc.html`, `Restore.dc.html`, `About.dc.html` — the same window in other states (each just mounts `Main` with different props).
- `Spec.dc.html` — tokens, type, components, the mythic layer, motion timings, layout and build notes. Treat it as the spec where it is more specific than the prototype.

To see them, open the `.dc.html` files' markup as reference. They need the canvas runtime to render, so don't try to build them. Pixel values in the markup are at the 600×450 base size.

## What to build

1. **Layout (base 600×450, fixed 4:3, resizable 400×300 → 1200×900).** Lay out at base size and scale uniformly (e.g. an `AffineTransform` on a content component). Zones:
   - Header y 10–46: sigil, KRONOS wordmark, hairline rule with a travelling glint, the Greek inscription ΧΡΟΝΟΣ · ΚΑΙΡΟΣ · ΑΙΩΝ, a TRACKING/PAUSED status chip with LED, a theme toggle (☉/☽ drawn as paths) and a menu button.
   - Readout panel at (16,56), 568×188, with chamfered corners. It holds:
     - the 160 px dial at left, with start/pause as the dial's centre button;
     - the project time readout (H:MM:SS, JetBrains Mono Light 54);
     - THIS RUN, plus a stats row: TODAY, DAILY AVG, LONGEST DAY.
   - Sessions panel at (16,256), 568×178:
     - a header with day count and a RECENT/LONGEST segmented control;
     - a list that scrolls natively, with rows showing the planetary weekday glyph, date, a bar scaled to the longest day, and the duration.
2. **Remove the old scroll up/down buttons.** Retire the old visual assets: the ~38 themed SVGs in `Resources/`, `Grit.jpg` and the ASTERA font. Remove them from `Kronos.jucer` BinaryData only once nothing references them.
3. **Draw everything in code** with `juce::Path` and `Graphics`: chamfered panels (12 px 45° cuts on opposing corners, 1 px line border), the dial rings, the hexagram, the sigil, the ouroboros, icons and the planetary glyphs ☉☽♂☿♃♀♄. Don't rely on host fonts for any symbol.
4. **Fonts:** embed Source Sans 3 (400/600/700, including the Greek glyphs) and JetBrains Mono (300/400/500) via BinaryData, under the OFL licence. Make sure the Greek renders.
5. **Theming:** put the dark and light token sets from `KR_THEMES` into LookAndFeel colour IDs, with dark as the default. Keep the existing `isDarkMode()`/`setDarkMode()` persistence.
6. **Dial** (radii from Spec "Dial anatomy"):
   - r74: seconds-in-minute arc in the accent colour, with a diamond at its tip.
   - r65: Greek inscription on a circle, rotating at 120 s per turn.
   - r57: minute-in-hour arc.
   - r50: hexagram inside Saturn's hexagon, counter-rotating at 72 s per turn.
   - The arcs must show real values. The decorative layers freeze when paused.
7. **New derived stats**, all computed from existing processor data (`timePerDate`, `startTime`, total):
   - today's time;
   - this run (elapsed since tracking started);
   - daily average (total ÷ days logged);
   - longest day.
   - Add accessors on the processor if needed, and add unit tests for them next to the existing embedded test suite.
8. **Menu → sheets** (in-window overlay with scrim, replacing the current full-bounds components):
   - **Backup:** project name field and a filename preview. Show the success state only after the file write has actually succeeded; on failure, show plain error text.
   - **Restore:** a list of `.kronos` backups plus Browse…, then a new **confirm step** that shows current vs incoming totals. Keep current is focused by default, Replace log is the primary button.
   - **About:** ouroboros emblem, version, credits, links (the URLs are in README.md).
   - Keep the existing backup/restore file logic. Change only the UI around it.
9. **Motion** (the Spec "Motion" table has exact timings):
   - repaint at 30 Hz only while the editor is visible and tracking; otherwise 1 Hz;
   - press 80 ms, hover 180 ms, settle 340 ms;
   - sheet arrival 340/760 ms with 80 ms offsets;
   - sort label decode;
   - a one-time pixel glitch on the wordmark when tracking starts;
   - LED breathing on a 2.4 s cycle;
   - the header glint on an 11 s cycle.
   - Ambient layers stop when the editor is hidden. Never play a success flourish before the operation has actually succeeded.

## Hard rules (founder preferences)

- No ruler/meterstick tick marks near the header. Keep motion slow and independent; don't put everything on one synchronized clock.
- The Greek text and glyphs are always secondary to the English labels. They never replace functional copy, and they stay out of accessibility descriptions. Engravings stay at 40–50% opacity in muted/faint tones; only the hexagon edge uses the accent colour.
- No invented telemetry or fake readouts. Everything shown is real data or plainly decorative.
- Colours must stay within the token sets: graphite, warm off-white, and the restrained accents #EF593D/#BC321E (dark) or #D2452B/#A82C19 (light). No blue from the old UI.
- Keep keyboard focus visible (2 px accent outline) and keep hit targets ≥32 px at base size.

## Keep working

These must keep working: tracking start/stop, per-day accumulation, midnight rollover, sort modes, state serialization, backup/restore and the theme toggle. Run `./scripts/run_tests_macos.sh` and keep it passing. Update `Kronos_Manual/Kronos Manual/Kronos Manual.md` and the README's feature list where the UI changed.

## Deliver

A branch with focused commits, passing tests, and before/after screenshots of the standalone app in these states: dark tracking, paused, light theme with the menu open, backup, restore confirm, and about, at 600×450 and 1200×900. Also write a short note listing any spec deviation and why.
