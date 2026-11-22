# Speed changes

These changes make DRI's weekly and offseason work faster without changing a recruiting, portal or roster decision. They are always on; there is nothing to configure. The reasons each one gives the same result, the one exception (an operating-system crash, under cache files), and the tests are given below.

## DRI's own cache files

DRI writes its own cache files with a temporary file, a complete-write check and an atomic `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` rename. This covers the HS plan cache (137 files after every regular-season week), the portal state and the head-coach hire record (`r3/hs34r3_continuity.inc`, `r3/rc4_portal_state.inc`, `r3/test36_opening.inc`). These three writers no longer call `FlushFileBuffers` on the temporary file and no longer add `MOVEFILE_WRITE_THROUGH` to the rename, so the step does not wait for the disk. They keep the temporary file, the check and the rename, so a reader still sees either the old file or the complete new one.

The files are only read back by DRI, in the same session or a later one, and Windows returns the same bytes from its cache or the disk. A game crash does not lose a file, because the data has already been handed to Windows. The only difference is an operating-system crash or power loss in the short time before Windows writes the file back. The file is then missing or keeps its previous version, and the reader takes the path it already takes when a file is missing or invalid (for the plan cache, `hs34r3_reconstruct`). That path can produce a different plan for that one week. A forced flush narrowed this window but did not close it: a power loss during the write had the same result before.

The EPS cache writer (`r3/eps_runtime.inc`) is left unchanged on purpose and always flushes.

## Roster pass ownership check

Before every move and cut, the preseason roster transaction pass checks that the player is owned exactly once, by the expected team (`rm_owned` in `roster/native_actions.inc`). That check walks all 138 teams' roster and arrivals arrays every time. The BEFORE check now answers from an index (`rm_owned_before` in `roster/roster_owner_index.inc`):

- The index is built at the first check of a pass, from one league read that uses `rm_owned`'s own calls and rules.
- After every executed action, whether it moved, cut, went stale or failed, the source and destination teams are marked. Both of their arrays are read again before the next check.
- The verdict follows `rm_owned`'s rule: no verdict if any array is unreadable or oversized; otherwise exactly one occurrence, in the expected team's roster array, or none when no owner is expected.
- If the index cannot be built or updated, that check falls back to `rm_owned`.
- The index is active only inside the transaction pass and is freed at its end.

The index gives the same answer as `rm_owned` as long as no team the pass did not touch changes during the pass. The pass runs synchronously inside the enrollment callback, and its only writes are its own moves, cuts and NIL payments on the source and destination. The AFTER and FINAL checks of every move and cut still use the full-league `rm_owned`, and the audits at the end of the pass are unchanged.

At the end of each pass the roster log gets one summary row:

```text
RM18R22 ROSTER_OWNER_INDEX queries=<checks> builds=1 rescans=<n> fallback=0
```

A normal pass shows `fallback=0` and `queries` equal to the number of moves and cuts.

## Tests

- `python3 scripts/run_portable_checks.py` also runs `tests/test_roster_owner_index.py`. That test compiles the production `rm_owned` line and `roster_owner_index.inc` against a modelled league and answers every query both ways, with the index and with `rm_owned`, over random passes. It also runs a third-team control whose difference it must report, and four broken copies of the index that it must catch.
- `source/r3/Tests/test34_r3_cache_io_test.py` runs the plan-cache writer against modelled Win32 file services: no flush, a plain `MOVEFILE_REPLACE_EXISTING` rename, and every injected failure leaves the previous file in place.
