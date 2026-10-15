# New Game Plus for macOS (Apple Silicon)

This folder builds New Game Plus (by not_alphanine) for the native macOS build of Cyberpunk 2077 (2.3.1, Steam,
Apple Silicon). Port by codeman101 (skyman3215). GPL-3.0, like the rest of the repo. SharedPunk stays MIT.

Everything macOS-specific is in this folder and in `#ifdef __APPLE__` blocks. The Windows build is meant to be
unchanged: `WINDOWS_CHECK.md` is the `unifdef -U__APPLE__` comparison against `main` at 15fb289 plus the one-line `"mac"` platform fix (SharedPunk against 5779848).
What is not done yet: `NOT_YET.md`.

## Status

`NewGamePlus.dylib` builds and links for arm64 and runs in the game: with a save loaded, the hooks install, the game system
registers, and the save scan, the save picker's check and the progression read (a PC end-game save and a Mac save) work in
automated runs. Starting a New Game Plus run has not been tried yet. Details and what is still missing: `NOT_YET.md`.

Features that need an address which is not in the Mac rows log one line and skip. Adding that row later turns the
feature on.

## Game updates

The macOS addresses (`NgpSdkRows.inc`, `hook-offsets.txt` and the offsets listed in `NOT_YET.md`) are for game 2.3.1.
A game patch moves them, so each patch needs new values. codeman101 (skyman3215) keeps them up to date: open an issue
and mention @codeman101.

## What you need to run it

- Cyberpunk 2077 2.3.1, the native macOS build (Steam), on Apple Silicon.
- A RED4ext loader for macOS that installs the plugin's hooks (`macos/hook-offsets.txt`) and calls
  `ngplus_registerNatives` after every plugin has loaded (at script initialisation). The port was developed against the same loader
  setup as the macOS ports of TweakXL, ArchiveXL, Codeware and Mod Settings.
- ArchiveXL for macOS. The archive is installed at `archive/Mac/mod/NewGamePlus.archive`, which ArchiveXL mounts at startup.
- TweakXL for macOS. Tweaks stay in the plugin folder and are registered from `ngplus_registerNatives`.

## Build

```
cmake -S macos -B macos/build -DCMAKE_BUILD_TYPE=Release
cmake --build macos/build -j8
```

Needs the Xcode command line tools, CMake 3.21+ and Homebrew `lz4` (the static library), plus these checkouts. Each has
a CMake variable; the defaults are paths next to this repo.

- `RED4EXT_SDK_DIR`: a macOS RED4ext SDK. The port was built against a local branch of jackmaxwil's RED4ext.SDK-macos
  with Mac ABI fixes; some of those are still open as PRs there.
- `CODEWARE_DIR`: Codeware for macOS (codeman101/cp2077-codeware, branch `macos`). Its `macos/MacSdkRows.inc` and its
  `gameIGameSystem` headers are used, and `NgpSdkRows.inc` is added to those rows.
- `SHARED_PUNK_DIR`: SharedPunk with its macOS changes (codeman101/SharedPunk, branch `macos`).
- `REDLIB_ROOT`: jackhumbert's cp2077-red-lib. `redlib/` here overlays the files that need macOS changes.
- `HOPSCOTCH_DIR`, `NLOHMANN_JSON_INCLUDE`: Tessil's hopscotch-map and nlohmann/json headers.

After a successful link, `fix-install-names.sh` signs ad hoc. Static lz4 is not a linked dylib, so nothing is copied
beside the plugin for it. `check-undefined.sh` checks the exports (`Main`, `Query`, `Supports`, `ngplus_registerNatives`)
and that no symbol is left for dynamic lookup.

## Install layout

`red4ext/plugins/NewGamePlus/`: `NewGamePlus.dylib`, the redscript sources under `Scripts/`, the tweaks, `LICENSE`,
`THIRDPARTY_LICENSES`, and `SharedPunk-LICENSE`. The archive and its `.xl` file go to `archive/Mac/mod/`.

- The scripts are compiled into the script cache by the loader setup. On macOS they are not added through RED4ext (`scripts->Add` is skipped).
- When `archive/Mac/mod/NewGamePlus.archive` is already present, `RegisterArchive` is skipped.
- Archive and tweak registration run from `ngplus_registerNatives`, not from `Main` at load. RTTI is not ready at load.

## How the macOS build differs

- **SDK.** `CMakeLists.txt` writes `build/sdk-overlay`: the macOS SDK address table, Codeware's Mac rows, and the
  New Game Plus rows in `NgpSdkRows.inc`. An unknown hash resolves to `ngplus_mac_trap_for_hash` (`MacShims.cpp`), which logs once
  and returns a stub. The stub returns 0. `IsAvailable()` is false for that stub, and `RawFunc` / `RawPtr` return a
  stand-in instead of calling or reading address 0.
- **DynArray length.** The Mac SDK stores the length in a field named `size`. The overlay's `NgpSizeWord` is four bytes,
  converts to `uint32_t`, and can be called, so upstream `.size()` still compiles.
- **Natives.** The game does not run RTTI register callbacks. `ngplus_registerNatives` runs the RedLib queue (`RunPending`
  in the Registrar overlay), then archive and tweak registration, then sets the RTTI pointers. A pool self-test runs first
  (`MacVault.cpp`). The async pool id is `0x7FF80000`. Serializable class bits are 62 through 54.
- **Hooks.** One attempt through the loader API. The log line says INSTALLED, or SKIPPED when the attach does not take
  (no API, refused, or the offset is not on the allowlist). There is no retry. `OrDie` and `RaiseAssert` log and return.
  They do not execute `int3`. `RawVFunc` adds 8 to the slot on Apple.
- **LoadFactMap.** The callback writes the three NG+ facts into the global at image base plus `0x8132140`. That global
  is a `HashMap` of `uint32_t` to `CString` (confirmed from the Mac function). The Windows callback writes the map
  argument instead.
- **Save metadata.** The game's metadata loader has no macOS entry, so `NgpMetadata.cpp` reads `metadata.9.json`
  directly (the fields New Game Plus uses).
- **Save node.** The game inlines its node accessor on macOS. SharedPunk's `Save.cpp` has an Apple copy that calls the
  stream's virtuals one slot later than Windows (`+0x98` check, `+0x78` open, `+0x80` close).
- **Calling convention.** SharedPunk's `src/code/Mac/FileAbi.cpp` (`shared::mac`) wraps the game functions whose
  macOS registers differ from its prototypes (results through `x8`, text as a pointer and a length).
- **IGameSystem.** The build uses Codeware's macOS `gameIGameSystem` header, where `IsSavingLocked` returns its lock
  through `x8`. The SDK's Windows signature crashes the game on the first save.
- **Left out of the Mac target.** `src/Parsing/New/` and `TestingDefs.cpp`. The quest-resource dedupe hook is not installed.
