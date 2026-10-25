# New Game Plus on macOS: not done yet

Addresses below are image offsets in the macOS 2.3.1 binary. What has run in the game is under "Checked in game" at
the end.

## Rows

Addresses the loader's database does not have yet. The first table's rows go to jackmaxwil/RED4ext.SDK-macos in one pull
request; with them added locally, the automated run on his loader passes (see the end).

| hash | offset | name |
|---|---|---|
| 3923848734 | 0x2ce34d0 | Telemetry LoadFactMap (hooked) |
| 2393518559 | 0x245733c | CharacterCustomization OnNewGame (hooked) |
| 4293661415 | 0x170cfe0 | GetSaveFolder. No arguments. `CString&` in x0 |
| 836113218 | 0x3449908 | MergeDirToPath. x0 source, x1 chars, w2 length, x8 result |
| 1194333091 | 0x344993c | MergeFileToPath. Same registers |
| 3788966949 | 0x90031f8 | RedFileManager instance (address of the pointer) |
| 1917464012 | 0x170e538 | OpenFileStream. x1 path, w2 flags, x8 result. x0 unused |
| 4051576167 | 0x170ec50 | FindFiles. x0 this, x1 root, x2 chars, w3 length, x4 array, w5 recurse |
| 1617892594 | 0x2ef9d9c | QuestsSystem::AddQuest (vtable +0x1f8) |
| 188748408 | 0x3f106c8 | SessionData AddArgument |
| 1357913210 | 0x3f339b8 | StartSession. Copies the session; the caller destroys its copy |
| 1002050525 | 0x3f2d4d0 | GameDefinition::ToWorldID. x0 definition, x8 result |
| 2635337807 | 0x8d5d830 | InkSystem instance (address of the pointer) |

`RaiseAssert` (4285205681) is not compiled into the SDK 1.0 build. `CGameEngine` (2549221846) is a verified row of his
database already (`3:0x1D483A0`, the same address).

The functions whose macOS registers differ from SharedPunk's prototypes are called through SharedPunk's `shared::mac` wrappers (`src/code/Mac/FileAbi.cpp`).

## Not on macOS, replaced

- **Save metadata.** `LoadSaveMetadataFromFile` (1649938065) has no entry. `NgpMetadata.cpp` reads `metadata.9.json`.
- **Save node accessor.** The ctor and dtor (3391557872, 2159741197) are inlined at every caller. SharedPunk's `Save.cpp`
  does the same steps on the stream (`+0x98`, `+0x78`, `+0x80`), checked against an inlined copy in the game.
- **SessionData destructor** (3460762581) is inlined. On macOS `StartSession` does not destroy the session it is given;
  `~Data` releases the argument array with the game's own helper at `0x3f0e794`, as the game does after `StartSession`.
- **Buffered stream destructor** (2577866805) is inlined into `OpenFileStream`. It is not called.
- **Quest resource dedupe.** `LoadQuestResources` (407969320) is inlined into `0x2ef743c`. The hook is not installed
  and the log says so. The middle of that function is not hooked.

## Still missing

- **Exterior detection for random encounters.** GetPlayerControlledGameObject (3420005096), World::IsInInterior
  (3384552122) and SafeAreaManager::IsPointInSafeArea (2322404505) have no row. `TickInteriors` and the
  `ngplus_is_outside` fact are skipped with a log line.
- **Loading-screen picture.** The loading-screen node's Execute (3489736669) and the loading layer setter (1370036335)
  have no row. Starting a run shows the game's default loading screen.

## Checked layouts

- QuestsSystem mutex at `+0x60`: the game's lock function (QuestsSystem vtable `+0x238`) takes `this+0x60`.
- FactsDBManager: its virtuals sit one slot later than on Windows (the quest load calls the Windows `+0x50` slot at
  `+0x58`), so SetFact is `+0x20`. SharedPunk's class has a virtual destructor first, and clang emits `+0x20`.
- InkSystem requests handler: the weak handle at `+0x370` (the game's own lock helper reads `+0x370` / `+0x378`).
- CharacterCustomization: lock `0x70`, state `0x78`, IsNewGame `0x200`, GetLifePath `0x148`.
- Quest list `0xA8` (count `0xB4`), facts pointer `0xF8`.

## Checked in game (automated)

Loading a save with the plugin on: both hooks install, the natives and the game system register, the save scan and the
resolve answer as on Windows (end-game saves are skipped by design; a Mac save that finished q104, q110 and q112 qualifies),
and the progression data of a PC end-game save and of a Mac save are read (level, street cred, perk points, money,
inventory and stash). Last run 2026-10-15: this xmake build on jackmaxwil's loader, with the rows above added to its
database, 8 of 8 checks.

## Checked in game (by hand, 2026-10-14)

Starting a New Game Plus run from the main menu, both start types (StartFromQ101 and StartFromQ001): character
creation, LaunchNewGamePlus and StartSession, loading into the world, and the progression transfer (development,
inventory, stash, equipped cyberware, garage, craftbook, stash retrofixes, facts). The first try crashed after character
creation: the Mac `gameGameSessionDesc` is 0x1B0 bytes, not the SDK's 0x1D0 (see `MacSessionDesc.hpp`). On 2026-10-15
the run's autosave was loaded back into the world (the save node). These hand checks used the earlier CMake build.

Script note: on macOS, `ArraySize()` applied directly to a native function's result (`ArraySize(obj.GetList())`) reads a
stale temporary. A local copy, a parameter, or `for x in obj.GetList()` (what the New Game Plus scripts use) is exact.

## Open

- Requests handler `InputDeviceId`: SharedPunk now reads `+0x6C8` (Windows 2.5's vanilla
  `StartNewGame` reads it there; 2.3.1 read `+0x630` through a getter on the handler-side
  object). The Mac 2.5 offset is still unverified.
- LoadFactMap writes three facts into the global at image base plus `0x8132140` (a `HashMap<uint32_t, CString>` filled
  to its capacity of 161 by the game). The inserts grow the map through its allocator. Not run yet.
- `src/Parsing/New/` and `TestingDefs.cpp` are not in the Mac target. The encounter-test console commands
  (`scripting/Tests/NGPlusEncounterTests.reds`) use natives from `TestingDefs.cpp`, so that script is left out of the
  Mac install too.
