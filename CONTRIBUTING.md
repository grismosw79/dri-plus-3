# Contributing

Use the current T3 source as the common starting point. Keep each compatibility investigation or bug fix on a separate branch and submit a pull request for the maintainer to review before it enters the release baseline.

Include the trigger, affected builds/mod combination, cause, exact change, tests actually run and remaining live-test needs. Keep unrelated tuning and gameplay changes out of a focused hotfix. Do not turn off identity or structural guards to make a test pass.

Run `python3 scripts/run_portable_checks.py` for changes touching the launcher, monitor or roster policy. Run the relevant subsystem and managed checks for their changes. A new test should demonstrate the reported failure or a real regression risk. State when a historical test cannot run because its external fixtures are absent.

Use copies of saves for in-game validation. Give candidate plugins distinct version labels, and keep the current release available for rollback. The maintainer decides when a candidate becomes a public release.

Keep executable captures, personal diagnostics, saves, credentials and build outputs out of commits. The repository is public for source review and compatibility collaboration. No open-source license has been selected.
