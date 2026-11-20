# Current release baseline

Prepared November 20, 2026 from `DRI-Plus-3.0-Nov19-2026-T3-Release.zip`.

- Source baseline: R22 TU1117 T3.
- Managed version: `3.0.22.3`.
- Informational version: `3.0.22.3-r22-tu1117-t3-steam-test`.
- Display label: `3.0 R22 TU1117 T3 Steam Test` (retained when promoted).
- Native gameplay payload: unchanged from T2.
- Source issue `portal_staff28_position`: open; no fix included.

## SHA-256 provenance

| Artifact | SHA-256 |
| --- | --- |
| Complete November 19 release ZIP | `2a034fbb52c1a11b5664e0868878e287e2dd7adf01b26da1ed9f8854e000f39d` |
| Nested T3 source ZIP | `71798fda7b36d6d7c1f758e9efb50f60db4713917767f17fdd421a959590ca97` |
| Installable `Plugins/DRIPlus3.dll` | `e0ef288b7afeef76fe6cb9593b3d6bd4ff3637238ba5a371bae2f5a0de177aeb` |
| Native `DRIPlus3-RM18R22.dll` | `d3899e8cd372ca73ce53adcc6e6a8919c06d3082e91174f51284c8775ffebfce` |
| Native `DRIPlus3-RM18R22.exe` | `a4cff9e12840b44d0833aca7645e8ab0fd0a9d604a4f2087ff5ab1fc45664144` |
| Matching November 17 `.fbmod` | `6b80d67ea249a4fd41ad8194e5b6a2e85f2fa1945b14c5c5e6ca5f632860fbd2` |

## Accepted game executable hashes

```text
3bc0317b57735ae9615051ea00c8840b0bcf895fdcfc7e8738c6565423f764c0
ed6b60934e818e291fd6802988f6dd64353f533d2410a2e03d95b2beeb5bc471
```

The second fingerprint was added in T3. There is no wildcard acceptance. Do not infer the storefront solely from the executable's directory name.

## What the evidence establishes

The existing T3 validation records report 138 identity cases, 76 mocked monitor scenarios, 28 managed startup checks and 32 compiled-plugin checks passing. The maintainer subsequently reported successful startup from both EA App and Steam users, and the complete release retained those exact T3 binaries.

That user report is not a new multi-season T3 test. No new individual success receipts or full Steam native-code capture were supplied at promotion. Existing historical source README/status strings contain earlier test-stage labels and are not current qualification results.

Initial repository preparation preserved all imported code bytes and ran the top-level portable checks again. A subsequent privacy pass removed one personal attribution from a code comment; all executable code remains unchanged. It does not rebuild, install or change the released DLL. See `PREPARATION-CHECKS.md` for that preparation's actual results.
