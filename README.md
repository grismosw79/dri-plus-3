# DRI+ 3.0

Dynasty Recruiting Intelligence for College Football 27. This repository is prepared for source review and compatibility work with Ace, Zip and Sdmart3.

The starting point is **R22 TU1117 T3**, promoted to the complete November 19, 2026 release after the maintainer reported successful startup from both EA App and Steam users. The native gameplay code is unchanged from T2. T3 adds a second exact executable fingerprint to the monitor and updates the managed plugin's metadata and payload pin.

**This is a development source tree.** The normal installation is the complete release package, using `Plugins/DRIPlus3.dll` together with the matching 3.0 mod and a prepared offline dynasty. Building this tree does not create a newly validated public release.

## Start here

- [Build instructions](docs/BUILD.md)
- [Compatibility review guide](docs/COMPATIBILITY.md)
- [Current release and validation](docs/CURRENT-RELEASE.md)
- [Open placeholder-record bug](docs/issues/001-staff28-placeholder.md)
- [Contribution workflow](CONTRIBUTING.md)

## Source map

| Path | Purpose |
| --- | --- |
| `frosty/DRIPlus3.cs`, `frosty/LaunchWatcher.cs` | Managed Frosty/MMC plugin, automatic launch, payload preparation and readiness handling |
| `source/launcher.c`, `source/launcher_monitor.inc` | Native launcher, executable identity checks and session monitoring |
| `source/combined.c` | Initialization and preflight coordination for native subsystems |
| `source/r3/` | Recruiting, portal, dealbreaker grades and coaching carousel logic |
| `source/positions_dev/` | Position changes and annual development |
| `source/roster/` | Roster management and specialist coverage |
| `source/capacity/`, `source/reserve/` | Roster capacity and reserve support |
| `scripts/` | Native build, managed payload pin synchronization and portable checks |
| `tests/`, `frosty/Tests/` | Portable native checks and managed offline checks |
| `source/r3/Tests/`, `Emergency-Tests/` | Historical test source; some tests require additional fixtures or Windows |

Of the 501 imported code files, 500 match the release source archive byte for byte. One file has only a personal attribution removed from a comment; executable code is unchanged. Original hashes and the sanitized file hash are recorded in [the source manifest](docs/release-source-manifest.json). Repository documentation and the portable test runner are new; no gameplay fix has been applied.

Captured game memory, save-derived fixture data, user diagnostics, compiled DLLs/EXEs/PDBs, old result logs and workspace-specific packaging scripts are omitted. Generated headers required by the native build are retained. Historical test code is retained for reference, but its entire suite is not self-contained.

The `.fbmod`, raw FTC exports and initial save-preparation app are separate project components and are not included in this DLL source tree. FTC comparisons need the matching November 17 tuning baseline separately.

Start with a private repository. Public distribution and an open-source license remain decisions for the maintainer; this preparation does not grant a new license.
