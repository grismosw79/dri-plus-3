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

## Cache stack

The weekly HS maintenance, the post-week HS block and the offseason portal callbacks re-read the same game state many times inside one synchronous block of DRI code: the same league scans, the same school's roster snapshot or portal room, the same (school, recruit) eligibility. The cache stack answers such a repeat from the first read, and only inside a block in which nothing can have changed the answer.

**Windows.** A cache answers only while a window is open. A window is one synchronous block on the game thread:

| Window | Code |
| --- | --- |
| `MAINT` | `hs28_maintenance` (the pre-plan HS maintenance) |
| `POST_WEEK` | the HS block of `DriAfterPortalWeekly`, paused around `t36_open` |
| `PORTAL_MAINT_RECOVER_MARKET`, `POST_PORTAL_RECOVER_MARKET` | the read-only plan loop of `portal_recover_market` (closed before its first write or removal) |
| `PORTAL_MARKET` | the portal market forecast in the CPU prepare hook |
| `FUND_OBSERVE` | the two post-funding observers of `DriAfterHsFunding` |
| `EXEC_OBSERVE` | the executed portal action / pitch observation |
| `START_PRUNE`, `PORTAL_WEEKLY`, `HS_WEEKLY` | the decision part of `DriOnStart`'s portal prune, `DriOnPortalWeekly` and `DriOnHsOwnership` (closed before any write of the callback; the prune closes it around every native removal) |

Windows do not nest and never span two hook calls.

**Validity.** Three counters decide how long an entry stays valid:

- the *generation* advances at every window open, close and pause, at every DRI game write (record setters, board admits, offers, actions, funding, publication), after every native board removal, at every PT28 index invalidation (which every ownership-ledger change performs) and at the entry of every native-to-DRI hook except the read-only grade, score and shortlist hooks that run inside the natives DRI itself calls;
- the *epoch* advances at every window open and after every step of the two HS windows;
- the *class epoch* advances at every `test29_class_invalidate`.

Every hook body outside `r3/eps_runtime.inc` starts with `cache_hook_entry(kind)` for this (`r3/cache_stack.h`). The three EPS grade hooks are read-only grade hooks, for which the call would do nothing, and `DriEpsUpdate` advances the generation through its first call, `pt28_invalidate`. `hooks.S` and `eps_runtime.inc` are unchanged.

**The caches.**

| Cache | What it keeps | Valid for |
| --- | --- | --- |
| Season scope (`r3/cache_stack.inc`) | year / week / type of the franchise season, read at window entry, for `hs28_native_score`, `hs28_scope`, `hs28_market_scope`, `portal_commit_scope` | the window, same eval and franchise |
| Owner index (`r3/cache_scans.inc`) | player -> owning team for `portal_published_owner`, built in one pass with the scan's rules | window and epoch |
| Secured list (`r3/cache_scans.inc`) | the ordered stage-5/6 entries of lists 39 / 46 for `hs15_secured` and `portal_commits` | window and epoch, until DRI writes a stage |
| PT28 roster (`r3/pt28_roster_cache.inc`) | the roster-array part of a PT28 room per (team, position group) | the PT28 index scope, team, roster array, departures |
| Offer capacity (`r3/offer_capacity_cache.inc`) | the other schools' boards for a run of one school's new-offer checks; the applicant's board stays live | until any hook other than the offer action, the grade hooks and that school's own action-loop hooks |
| Eligibility, recovery cost (`r3/cache_elig.inc`) | native eligibility per (school, recruit); the two recovery costs | one generation |
| HS snapshot (`r3/cache_snapshot.inc`) | `hs15_collect` per school | one generation (not counting stage writes below 5 and HS preference re-ordering, which a snapshot never reads) and one epoch |
| Portal room (`r3/cache_room.inc`) | `portal_rosters_collect` for one position per school | one generation, epoch and class epoch |
| PT28 reads (`r3/pt28_grade_reads.inc`) | the franchise, season and player fields one `DriPt28Grade` call reads more than once | the call |

The memos store only *clean* calls: the call returned successfully, wrote no log row, moved no counter and did not fault. Failed and unclean calls always run live, so every fault and diagnostic row is reached as before. The memos also answer only on the thread that opened the window. Together the tables take about 50 MB, allocated on first use.

After each post-week and post-NSD hook the DRI log gets one row per cache:

```text
CACHE_STACK_SUMMARY,POST_WEEK,<year>,<type>,<week>,FAST,ELIGIBILITY,queries=..,hits=..,compares=0,mismatch=0,stale=0,unclean=..
```

`stale` counts cached stage-5/6 entries that no longer qualified when they were used and must stay 0.

## Verification builds

The shipped build has no switch: `cache_stack_mode` (`r3/cache_stack.h`) is the constant `CACHE_FAST`, so the original-code and comparison branches of the cache stack compile away. For verification only, a build with `-DDRI_SPEED_TEST_MODES` makes it a variable with two more values. `scripts/build.py` never sets this macro, and such a build is not for release.

- `CACHE_OFF` runs the original code paths; every cache entry point falls through to them before touching any cache state, nothing is allocated and no `CACHE_STACK_` row is written.
- `CACHE_SHADOW` runs the original code, which decides, computes the cached answer beside every call and writes `CACHE_STACK_MISMATCH,<cache>,<window>,...` for every difference (at most 2,000 rows). A clean `shadow` season has no `CACHE_STACK_MISMATCH` row and `mismatch=0` in every summary row. `shadow` does all the original work plus the compares, so do not use it for timing.

The start value is `-DDRI_TEST_CACHE_STACK=<0|1|2>` (default 2, fast); the property test also switches it at run time. To compare a fixture or runtime DLL with the original code, build it twice, for example with `-DDRI_SPEED_TEST_MODES -DDRI_TEST_CACHE_STACK=1` for `shadow`, and compare the DRI logs without the `CACHE_STACK_` rows.

## Tests

- `python3 scripts/run_portable_checks.py` also runs `tests/test_roster_owner_index.py`. That test compiles the production `rm_owned` line and `roster_owner_index.inc` against a modelled league and answers every query both ways, with the index and with `rm_owned`, over random passes. It also runs a third-team control whose difference it must report, and four broken copies of the index that it must catch.
- `source/r3/Tests/test34_r3_cache_io_test.py` runs the plan-cache writer against modelled Win32 file services: no flush, a plain `MOVEFILE_REPLACE_EXISTING` rename, and every injected failure leaves the previous file in place.
- `python3 scripts/run_portable_checks.py` also runs `tests/test_cache_stack.py`. It compiles the production cache-stack framework, the owner index, the secured list, `portal_published_owner_live` and `hs15_secured` as a verification build against a modelled league and checks `fast` and `shadow` against the original code inside random windows, two controls that `shadow` must report, `off`, and the hook-entry rules.
