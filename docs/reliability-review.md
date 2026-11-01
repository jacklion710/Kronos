# Reliability review, 2026-10-31

Baseline: `origin/main` eb380f2 (1.1.0). Work is on `fix/long-run-reliability`, uncommitted.

## Fixed, each shown failing first

- Hang: `addElapsedSecondsAcrossDates` looped forever on any fractional elapsed time. Sampled on the real 1.1.0 binary at 100% CPU on the main thread.
- Lost time: each tick dropped its fraction. A simulated week of jittered ticks tracked 304,112s of 604,812s.
- Sleep counted as work: 9 hours asleep was credited in full. Gaps over 5 minutes now credit nothing.
- Invented time: restoring a tracking state or changing the parameter from the host after a pause credited the whole pause.
- Wrong days: day lengths assumed 24 hours, so catch-ups around DST changes landed on the wrong date.
- Time zones: reopening in another zone showed 2,400 of 3,000s and dropped a day. Date keys are now the source of truth.
- Corrupt or foreign state: foreign XML reset parameters, negative values loaded, and a total below its own days stayed wrong.
- Thread safety: tracking data is now locked, because hosts may save off the message thread.
- Restore UI: BROWSE destroyed its chooser immediately, success messages were wiped, and invalid files reported success.

## Efficiency, Release, before and after

| Measure | Before | After |
| --- | --- | --- |
| CPU, editor open, 60s | about 12.7% of a core | about 1.9% |
| Paint, dark | 160ms | 37ms |
| Paint, light | 143ms | 23ms |
| Tick and sort, 30 years of days | 567ms | 18ms |

Snapshots differ by a mean of 0.04 of 255 per channel.

## Requirements for the redesign

- `Time Display Dark.svg` embeds a 7510x3956 PNG, about 113MB decoded. Footprint is still 312MB, peak 557MB, and the editor takes about 1.2s to open. Keep embedded rasters at or below 1200px.
- Do not set component bounds or label text inside `paint()`.
- Read tracked data only through `getSortedDates`, `getTimeForDate`, `getLongestDaySeconds` and `getTotalTimeInSeconds`.
- Use `restoreStateFromData` for restores so invalid files are reported.

## Validation

- Long-run suite: passes in Debug and Release, three consecutive Debug runs after the final fix.
- Real-clock soak: 1,802s tracked against 1,801s of wall clock over 30 minutes on the real JUCE timer.
- pluginval strictness 10: all tests passed on the VST3, GUI tests skipped.
- UBSan: 0 failures and 0 runtime errors.
- Guard malloc: no memory faults. One timing assertion failed at 449ms against a 250ms budget, under its roughly 15x slowdown. Its large allocation warnings were the old 100MB and 118MB image decodes.
- Independent code review: found a regression that rejected pre-February 2025 state, backups with no time data reporting success, impossible dates passing validation and a stale-parameter race. All four are fixed with tests. Hardening its concurrency test then caught a stale-clock rewind in the race fix, also fixed.

## Not verified

- ThreadSanitizer and AddressSanitizer: both crash in `__pthread_init` on this Intel Mac with macOS 26, even on a three-line program. Locking has only the stress test and code review behind it.
- auval: not run. `auval -a` hung for over seven minutes on this machine, and validating an AU needs it installed.
- Nothing has been tested inside a DAW.
- Windows: the tests are unverified there.

## Known limits

- Up to 1s of carried fraction is lost per project reopen, about 55s over two simulated years.
- A sleep under 5 minutes is credited as work, and a message thread blocked for over 5 minutes credits nothing. The threshold is `maximumCreditableGapMs`.
- The old Restore screen still shows its status text in the title and can stay blank after restoring a shorter list. Both are in UI that the v2 redesign replaces.

## Warning for local builds

Release and Debug builds of the Xcode VST3, VST and AU targets copy into `~/Library/Audio/Plug-Ins`, replacing what is installed, whatever `SYMROOT` is set to. Build only the standalone target for testing, or remove the copy step in a scratch copy of the project.
