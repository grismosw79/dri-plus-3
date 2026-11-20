# Compatibility review

Start with the released T3 source and reproduce a reported failure on a copy of the affected dynasty. Record the game executable hash, DRI build, manager version, other mod versions/load order and the action immediately preceding the failure. Reuse an existing diagnostic report when it already answers these questions.

## Code entry points

| Concern | Start reading |
| --- | --- |
| Which executable is accepted | `source/game_identity.h`, `source/launcher.c` |
| Automatic activation and stale/duplicate sessions | `frosty/LaunchWatcher.cs`, `frosty/DRIPlus3.cs` |
| Native preflight and subsystem initialization | `source/combined.c` |
| Recruiting hook signatures and installation | `source/r3/signatures.h`, `source/r3/retention.c`, `source/r3/hooks.S` |
| Coaching carousel sites | `source/r3/car35_sites.inc`, `source/r3/car35_install.inc`, `source/r3/car35_hooks.inc` |
| Grade routing / placeholder records | `source/r3/eps_runtime.inc`, `staff28_runtime.inc`, `pt28_runtime.inc` |
| Position/development guards | `source/positions_dev/native_profile.h`, `source/positions_dev/rm_install.inc` |
| Roster hooks and writes | `source/roster/native_profile.h`, `source/roster/rm_install.inc`, `source/roster/native_actions.inc` |
| Capacity/reserve guards | `source/capacity/native_guard.h`, `source/reserve/native_guard.h` |

When comparing another mod, check both overlapping native hook sites and changes to the data/layout assumptions consumed by DRI. Record the address or field, the expected bytes or structure, which component changes it and when. Successful startup alone does not prove the two mods remain compatible through offseason transitions.

The release accepts exactly two executable hashes (see `CURRENT-RELEASE.md`). Accepting a hash is only one gate; native guards still apply. A Monitor 6 identity rejection, an activation failure and an in-play runtime fault are separate paths and should not be combined into one diagnosis.

For a proposed compatibility fix, describe the concrete trigger, old behavior, new behavior and test evidence. Preserve guard failures for unsupported layouts and real structural errors. Put the fix on its own branch and test a candidate plugin against the reported combination before a release decision.

Avoid putting whole game executables, captured memory, user saves or complete diagnostics in Git. Use minimal synthetic fixtures or a narrowly relevant redacted excerpt. The default repository contains no captured game binary or user save.
