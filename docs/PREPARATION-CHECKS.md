# Source preparation checks — November 20, 2026

| Check | Result |
| --- | --- |
| Complete release archive SHA-256 | Matches the recorded November 19 T3 release |
| Nested source archive SHA-256 | Matches `71798fda7b36d6d7c1f758e9efb50f60db4713917767f17fdd421a959590ca97` |
| Imported source fidelity | 500 of 501 imported code files match their source-archive bytes; one comment attribution was sanitized with executable code unchanged |
| Quoted production include dependencies | No missing local includes found in production C/header/include/assembly files |
| Exact executable identity cases | 138 passed |
| Release monitor with Win32 mocks | 76 scenarios passed, plus malformed/identity code checks |
| Roster policy regression | 17 suites passed |
| Specialist-room checks | Passed placement, rollback, coverage, scope and repeated-quote recovery cases |

The portable checks used GCC on Linux with assertions enabled. Compilation emitted existing indentation and initializer warnings in the imported source; it completed successfully. Source formatting was preserved instead of making unrelated changes during repository preparation.

A scan for common credential markers and personal email/local-user paths found no matches in the imported source archive's text files. This is a limited preparation check, not a comprehensive security audit. Binary captures, save-derived fixture data, compiled payloads, old logs and unrelated release-packaging scripts were excluded by the source selection.

No new native DLL build, managed plugin build, Windows launch, mod-combination run or live dynasty simulation was performed in this preparation. Earlier T3 results are identified separately in `CURRENT-RELEASE.md`. No open gameplay issue has been fixed as part of this source-sharing work.

The privacy pass removed the prior account name and personal name from repository documentation and one source comment. The source-comment change was verified against the original: no text outside that comment changed. The package contains no Git history. Commit identity still needs to be set to the chosen DRI alias and GitHub-provided noreply address before the first commit.
