# Kronos v2 spec deviations

Where the handoff, the spec and the prototype disagree, the hard rules win, then the spec, then the prototype. Everything else follows `Main.dc.html`.

## Resolved conflicts in the design

| Item | Design says | Built | Why |
| --- | --- | --- | --- |
| Light theme CTA | Prototype hard-codes `#BC321E` | `#A82C19` | The colour rule limits light to `#D2452B` and `#A82C19` |
| Minute arc | Prototype divides by a radius 64 circumference on a radius 57 ring, so it fills early | True fraction of the hour | "The arcs must show real values" |
| Dial inscription | Prototype uses full-strength faint | 50% opacity | Engravings stay at 40-50% |
| Hit targets | RECENT/LONGEST are 24px high, Close is 28px | Drawn at those sizes inside 32px targets | Targets must be at least 32px |
| Link hover | Prototype CSS uses `#FF7A5F` | A mix of existing tokens | Colours stay within the token sets |
| About version | Prototype shows v1.0.4 | The real build version | No invented readouts |
| Backup file | Prototype shows `MY PROJECT_2026-10-31.kronos` in `~/Documents/Kronos/Backups` | `MY PROJECT.kronos` in the existing Application Support folder | "Keep the existing backup/restore file logic" |
| Overwriting a backup | Not in the prototype | A confirm stage in the Backup sheet | Existing behaviour, kept with the file logic |

## Simplified from the spec

- **Sheet arrival:** the scrim fades and the sheet rises 12px over 340ms, and the kicker's accent rule draws over 760ms. The rule draws in place, not out from the menu button, and the 80ms offsets are not applied to the sheet's contents.
- **Theme swap:** a 340ms cross-fade from a snapshot. The panels do not settle with reading-order offsets.
- **Sort switch:** the label decodes in 180ms and rows fade in with a 40ms stagger. Rows do not travel to their new positions.
- **Reduced motion:** the prototype honours `prefers-reduced-motion`. JUCE has no cross-platform equivalent, so this is not implemented.
- **Readout label row:** the Greek tracking is .2em, not .3em, because both labels need to fit in 342px on one line.

## Fonts

- **Source Sans 3 Bold is Latin only.** No full Bold was on this machine, so this is the web build's Latin subset converted to TTF. The UI never sets Greek at weight 700, so nothing renders wrong, but it does not meet "400/600/700, including the Greek glyphs". Swap in Adobe's release TTF before shipping.
- **JetBrains Mono** weights 300, 400 and 500 are static instances made from the variable font.
- **OFL:** the licence texts are in `Resources/fonts`. The installers must include them when the fonts ship.

## Needs a decision

- **Credits:** the old About panel credited Aznadel for graphics and linked SoundCloud. The prototype drops both, so they are gone here. The README still has the credit.
- **CPU at 30 Hz:** tracking with the window open costs about 10.8% of one core on this i9, mostly Core Animation. Paused costs about 1.2%, and a closed editor costs neither. `ambientFrameRate` in `PluginEditor.cpp` is the one line to change if 15 Hz looks smooth enough.

## Screenshots

The handoff asks for screenshots of the standalone app. These are rendered off-screen by each build's real editor inside the standalone binary, because screen capture is not permitted on this Mac and computer use was declined. The prototype's sample days are used, padded to 27 logged days, so the time shown is seeded. Ambient layers are at rest and the old UI cannot show its native pop-up menu off-screen, so its `light-menu` shot has the menu closed.

## Not verified

- No click, hover, focus ring, animation or live resize was checked in a real window. Hit testing and control behaviour are covered by unit tests at 400, 600 and 1200 wide.
- `Builds/VisualStudio2022` was edited by hand because a Projucer re-save removes a custom post-build step and rewrites paths. The Windows build has not been run.
