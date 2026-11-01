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

## Not done

- ThreadSanitizer and ASan/UBSan runs. The concurrent save scenario has only run unsanitized.
- pluginval and auval on the fixed build. Nothing has been tested inside a DAW.
- The real-clock soak. `KRONOS_SOAK_SECONDS` is implemented but has not been run.
- Independent review of this diff, which was stopped before it reported.
- `scripts/run_tests_windows.ps1` still times out at 120s and the tests are unverified on Windows. The Debug suite takes about 75s on this Mac.
- `Builds/MacOSX` and `JuceLibraryCode` are stale and report 1.0.3 or 1.0.4, so re-run Projucer before a release.
- Known limit: up to 1s of carried fraction is lost per project reopen, about 55s over two simulated years.
