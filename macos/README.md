# New Game Plus for macOS (Apple Silicon)

This folder builds New Game Plus (by not_alphanine) for the native macOS build of Cyberpunk 2077 (2.3.1, Steam,
Apple Silicon). Port by codeman101 (skyman3215). GPL-3.0, like the rest of the repo. SharedPunk stays MIT.

Everything macOS-specific is in this folder and in `#ifdef __APPLE__` blocks. The Windows build is meant to be
unchanged: `WINDOWS_CHECK.md` is the `unifdef -U__APPLE__` comparison against `main` at 15fb289 plus the one-line `"mac"` platform fix (SharedPunk against 5779848).
What is not done yet: `NOT_YET.md`.

## Status

`NewGamePlus.dylib` builds with xmake and runs in the game on jackmaxwil's loader: the hooks install, the game system
registers, and the save scan, the save picker's check and the progression read (a PC end-game save and a Mac save) pass
in automated runs. A New Game Plus run was started, saved and loaded by hand (with the earlier CMake build of the same
code). Details and what is still missing: `NOT_YET.md`.

Thirteen of the addresses New Game Plus uses are not in the loader's address database yet; a pull request to
jackmaxwil/RED4ext.SDK-macos adds them. Until then the features that need them log one line and skip.

## Game updates

The macOS addresses (the address database, `hook-offsets.txt` and the offsets listed in `NOT_YET.md`) are for game 2.3.1.
A game patch moves them, so each patch needs new values. codeman101 (skyman3215) keeps them up to date: open an issue
and mention @codeman101.

## What you need to run it

- Cyberpunk 2077 2.3.1, the native macOS build (Steam), on Apple Silicon.
- The RED4ext loader for macOS, jackmaxwil/RED4ext-macos, with its address database. The plugin registers
  `ngplus_registerNatives` as a `CRTTISystem` post-register callback in `Main`.
- ArchiveXL for macOS. The archive is installed at `archive/Mac/mod/NewGamePlus.archive`, which ArchiveXL mounts at startup.
- TweakXL for macOS. Tweaks stay in the plugin folder and are registered from `ngplus_registerNatives`.

## Build

```
xmake f -p macosx -a arm64 -m release -y
xmake build
```

Needs the Xcode command line tools and xmake. Dependencies come through alphanin9's cp2077-xmake-repo (macosx recipes
pending, the patch is attached to the pull request): RED4ext.SDK (jackmaxwil/RED4ext.SDK-macos), RedLib, ArchiveXL and TweakXL
headers, SharedPunk, lz4, hopscotch-map and nlohmann_json. The RedLib, ArchiveXL and TweakXL macOS changes are applied
by the recipes.

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

- **SDK.** Addresses resolve from the loader's database (jackmaxwil's RED4ext.SDK-macos). A hash the database lacks (or
  has unverified) resolves to 0, `IsAvailable()` is false, and `RawFunc` / `RawPtr` return a stand-in instead of calling or reading address 0.
- **Game system.** The game creates the GameInstance before the engine stores its framework, so RedLib's system
  registration finds no instance. `NgpGameInstance.cpp` chains onto Codeware's GameInstance constructor hook and adds the
  New Game Plus system to the new instance itself.
- **Natives.** `Main(Load)` calls `Red::TypeInfoRegistrar::RegisterDiscovered()` and adds `ngplus_registerNatives` as a post-register
  callback, so the loader's RTTI callbacks run RedLib's queue. `ngplus_registerNatives` then does archive and tweak registration and sets
  the RTTI pointers. The async pool id is `0x7FF80000`. Serializable class bits are 62 through 54.
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
- **IGameSystem.** The build uses the `gameIGameSystem` header in `macos/inc`, where `IsSavingLocked` returns its lock
  through `x8`. The SDK's Windows signature crashes the game on the first save.
- **Left out of the Mac target.** `src/Parsing/New/` and `TestingDefs.cpp`. The quest-resource dedupe hook is not installed.
