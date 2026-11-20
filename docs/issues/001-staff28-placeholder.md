# Open: placeholder-like player record disables main recruiting logic

Reporter: Sdmart3. Status: **reported and reproduced offline; not fixed in T3**.

## Report

During a simulation from 2026 Week 0 toward 2027 preseason, the reporter hit this twice in separate runs around the late offseason/coaching carousel:

```text
FAULT,portal_staff28_position,behavior_disabled_restart_game
pos=86 player_row=0 team_index=2147483647 ovr=0 class_year=0 home_state=4294967295
```

The reporter described the record as empty. The unset-looking team/state values support that interpretation, but the record's actual lifetime and identity still need to be confirmed. Row zero alone is not a valid way to identify an empty record.

## Current code path

`staff28_grade_locked` in `source/r3/staff28_runtime.inc` faults when the position is outside 0..21. `portal_fault` sets shared R3 state to `faulted=1`, `mode=-1`. The current grade query falls back to the native grade through `DriEpsGrade`, but subsequent custom R3 recruiting/carousel behavior remains disabled until a fresh game process.

The separate roster, capacity and position/development units are not all equivalent to that R3 fault flag. Do not describe this as every DLL subsystem unloading.

A prior offline reproduction used the actual staff-grade/router code with mocked game records. It reproduced the invalid-position fault and the disabled next valid query, and checked valid row-zero players, specialist fallback, unstarted-league fallback and reference balance. It did not run inside CFB27. The reproduction harness itself is not in this source archive.

The comparison with other grade paths needs care: `eps_profile` mainly rejects an invalid archetype; it does not reject every invalid position. `pt28_player` returns failure for invalid inputs, but its outer wrapper can still call `portal_fault("PT28_inputs")`.

## Proposed scope

Recognize a confirmed placeholder/unavailable record, return to the existing native grade for that query, and keep the next valid custom query active. Use bounded diagnostics if needed. Preserve structural and memory-safety guards for real errors. Check the related PT28 outer path without broadening the patch into a general bypass for all invalid inputs.

## Acceptance checks before release

- Reproduce the reported placeholder case and confirm native fallback without setting the global R3 fault.
- Verify normal players, a legitimate row-zero player and K/P/LS still follow their intended paths.
- Preserve real structural-error handling and reference balance.
- Verify a subsequent valid query still uses the custom grade.
- Have one affected tester repeat the failing offseason path on a copy of the save, continue beyond it and save/reload successfully with DRI still active.

This is a targeted hotfix candidate. A successful compilation or ordinary startup alone is not enough to call it resolved. No new dynasty or long multi-season test is required just to check this reported path.
