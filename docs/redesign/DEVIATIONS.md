# Kronos v2 spec deviations

Where the handoff, the spec and the prototype disagree, the hard rules win, then the spec, then the prototype. Everything else follows `Main.dc.html`.

## Founder change: no Greek words

Jacob reviewed the build on 2026-11-01 and removed all Greek words, because no one will understand them. This overrides every Greek label in the handoff, spec and prototype.

- The header inscription is gone, along with its divider, and the hairline rule now runs from the wordmark to the status chip.
- Menu items, sheet kickers and the readout labels are English only.
- About no longer has the motto strip.
- The dial's turning ring at radius 65 now carries the seven planetary symbols with beads between them, in faint at 50%. That replacement is my choice, not from the design.
- The sort label decode uses punctuation in place of Greek letters.
- The sigil, ouroboros, hexagram and planetary day symbols stay because they are not words.

## Founder change: About

Jacob reviewed the About sheet on 2026-11-01. Discord is removed, Website points to jacklionaudio.com, and Aznadel is credited for the V1 design. The three remaining links share one row. SoundCloud stays removed, as in the prototype.

## Resolved conflicts in the design

| Item | Design says | Built | Why |
| --- | --- | --- | --- |
| Light theme CTA | Prototype hard-codes `#BC321E` | `#A82C19` | The colour rule limits light to `#D2452B` and `#A82C19` |
| Minute arc | Prototype divides by a radius 64 circumference on a radius 57 ring, so it fills early | True fraction of the hour | "The arcs must show real values" |
| Hexagram nodes | Prototype uses an opaque accent dot and opaque muted dots | Muted at 45% | Only the hexagon edge uses the accent |
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
- **Status chip:** the prototype sizes the chip to its text, which pushes the inscription and rule sideways on every state change. Here it stays TRACKING-width in both states, and the label, LED and glint cross-fade over 340ms.

## Fonts

- **Source Sans 3 Bold is Latin only.** No full Bold was on this machine, so this is the web build's Latin subset converted to TTF. With no Greek left in the UI nothing depends on the missing glyphs, but swap in Adobe's release TTF before shipping.
- **JetBrains Mono** weights 300, 400 and 500 are static instances made from the variable font.
- **OFL:** the licence texts are in `Resources/fonts`. The installers must include them when the fonts ship.

## Needs a decision

- **CPU at 30 Hz:** tracking with the window open costs about 10.8% of one core on this i9, mostly Core Animation. Paused costs about 1.2%, and a closed editor costs neither. `ambientFrameRate` in `PluginEditor.cpp` is the one line to change if 15 Hz looks smooth enough.

## Screenshots

The handoff asks for screenshots of the standalone app. These are rendered off-screen by each build's real editor inside the standalone binary, because screen capture is not permitted on this Mac and computer use was declined. The prototype's sample days are used, padded to 27 logged days, so the time shown is seeded. Ambient layers are at rest and the old UI cannot show its native pop-up menu off-screen, so its `light-menu` shot has the menu closed.

## Not verified

- No click, hover, focus ring, animation or live resize was checked in a real window. Hit testing and control behaviour are covered by unit tests at 400, 600 and 1200 wide.
- A host that hides its window without minimising it may keep the ambient layers running, because `isShowing()` only sees JUCE's visibility flags and minimised state. No host has been tested for this.
- `Builds/VisualStudio2022` was edited by hand because a Projucer re-save removes a custom post-build step and rewrites paths. The Windows build has not been run.
