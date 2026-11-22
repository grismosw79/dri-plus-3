# Building DRI+ 3.0

Run commands from the repository root. Python 3 is required. Build outputs are ignored by Git. Source files retain their original bytes and line endings.

## Native payloads

Use Zig **0.13.0** for the recorded native build path. The build targets Windows x64 and can be invoked from a supported Linux or Windows host:

```text
python scripts/build.py --zig /absolute/path/to/zig
python scripts/sync_frosty_payload.py
```

On Windows, pass the full path to `zig.exe`. The first command creates `Runtime/DRIPlus3-RM18R22.dll` and `Runtime/DRIPlus3-RM18R22.exe`, and updates `source/dll_hash.h`. The second updates payload hashes in `frosty/DRIPlus3.cs` and `frosty/BUILD.ps1`. Review these changes alongside the actual source change; the hash checks are intentional.

The script also accepts `--launcher-only`. That option **requires an existing native gameplay DLL** in `Runtime/`; it cannot be the first build in a clean checkout. For a launcher-only patch to the shipped release, use the exact released native DLL and verify its hash against `CURRENT-RELEASE.md` before rebuilding the launcher. A full rebuild has not been asserted to reproduce the released payload bytes.

## Installable managed plugin on Windows

Use the real `FrostyCore.dll` and `FrostySdk.dll` from the supported MMC/Frosty installation. `BUILD.ps1` uses the Windows .NET Framework C# compiler and checks both native payload hashes before embedding them.

```powershell
powershell -File frosty/BUILD.ps1 -MmcFolder 'C:\path\to\MMC' -CandidateRuntime '.\Runtime'
```

Output: `frosty/build/DRIPlus3.dll`. This managed DLL is the plugin installed in MMC's `Plugins` directory. The native DLL in `Runtime` is a different component; do not rename it into a plugin.

For a candidate intended for testers, update its managed version/informational labels so it can be distinguished from T3. Review the expected version assertions in the managed checks too. Keep an untouched released plugin for rollback, and close the game and manager before swapping plugins.

## Portable checks

On Linux with GCC and Python 3:

```text
python3 scripts/run_portable_checks.py
```

The runner compiles and executes the four existing top-level C checks: exact game identity acceptance/rejection, the release monitor with Win32 mocks, roster policy regression and specialist-room repair. It then runs `tests/test_roster_owner_index.py` and `tests/test_cache_stack.py`, the property tests for the roster pass ownership index and the cache stack (see [Speed changes](SPEED.md)). It writes binaries and logs beneath `build/portable-checks/` and stops on failure. These checks do not launch the game or exercise real Windows injection.

## Managed offline checks

With a .NET SDK supporting .NET 8 and a .NET Framework 4.6.2 reference pack, after creating both native payloads:

```text
python frosty/Tests/build_offline.py --dotnet /absolute/path/to/dotnet --references /absolute/path/to/net462/references --output build/managed-checks
```

The reference path must contain `mscorlib.dll`, `System.dll` and `System.Core.dll`. These checks compile against the included RC3 API stubs. They are distinct from a Windows build against real manager assemblies. **Never distribute the stub FrostyCore/FrostySdk assemblies as MMC dependencies.**

## Historical test limitations

`source/r3/Tests/` and `Emergency-Tests/` include earlier test programs. Some require captured native code, original workspace layouts, save-derived JSON/GZIP fixtures, external parsers, Windows or additional toolchains. Those inputs are not included here. There is no claim that every historical test runs from a clean checkout. Reuse the relevant test logic or add a focused synthetic reproduction for the proposed fix.

No new Windows/game build or live simulation was performed while preparing this repository. Use `CURRENT-RELEASE.md` and the issue-specific acceptance criteria to separate existing release evidence from new candidate validation.
