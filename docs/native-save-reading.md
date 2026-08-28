# Reading save nodes the native way

A practical guide to reading each save node through the game's own load stream, for the V2 parser
described in [save-loader-v2-migration.md](save-loader-v2-migration.md).

Every recipe below is derived from the game's own `OnGameLoad` for that system, so the addresses
are the authority if a node's layout ever comes into question. VAs, image base `0x140000000`.

---

## 1. Primitives

```cpp
#include <Shared/Raw/Save/Save.hpp>
using namespace shared::raw;
```

| primitive | what it does |
| --- | --- |
| `Save::Stream::LoadStream::Create(stream, metadata)` | wraps a `BufferedRedFileStream` + `save::Metadata` |
| `loadStream.Initialize()` | must be called before any read |
| `Save::NodeAccessor(stream, name, aUnk1, aIsOptional)` | RAII scope onto one node; `IsGood()` says whether it resolved |
| `loadStream.ReadPackage(CClass*)` | reads a node that is a serialized package → `Handle<ISerializable>` |
| `loadStream.ReadBuffer()` | reads the node as a `RawBuffer` for manual/package-reader decoding |
| `loadStream->ReadWriteEx(&field)` | typed field read off the underlying `BaseStream` |
| `loadStream.GetSaveVersion()` / `GetGameVersion()` | version gates |
| `loadStream.IsGood()` | check after every logical step; the stream latches failure |

`LoadStreamContainer::Setup()` in `Parsing/New/ParserV2.cpp` already does the create/initialize
dance — use it rather than re-deriving it.

`NodeAccessor`'s last two arguments are consistently `(1, 0)` for required nodes across every
system I looked at. `PersistencySystem` passes `(0, 0)` for its first, optional header node, and
`StatsSystem` passes a computed flag as `aIsOptional` to skip the node on old saves.

## 2. The three strategies

The game reads nodes in one of three ways. Identify which one applies before writing any code —
picking wrong is the difference between four lines and four hundred.

### A. Package → RTTI handle

The node *is* a serialized RTTI object. One call, no manual parsing. **Best case.**

```cpp
Save::NodeAccessor node(aStream, GetName(), true, false);
if (!aStream.IsGood()) return false;

m_handle = aStream.ReadPackage(GetClass<game::StatsStateMapStructure>());
if (!m_handle) return false;

auto* s = reinterpret_cast<game::StatsStateMapStructure*>(m_handle.instance);
```

The `reinterpret_cast` is unavoidable today — `ReadPackage` returns
`Handle<ISerializable>` and the concrete type is not a real RTTI parent. `Nodes/StatsSystem.cpp`
notes this as *"Ugly, but inheritance is fucked on this struct"*; the V1 node says the same.

### B. Raw buffer → scriptable package reader

The node is a *package container* holding many objects addressed by index.

```cpp
Save::NodeAccessor node(aStream, GetName(), true, false);
auto buffer = aStream.ReadBuffer();

ScriptablePackage::ScriptablePackageReader reader(buffer);

PackageHeader header{};
reader.ReadHeader(header);
reader.ReadHeader(header);   // yes, twice - the game does this

ScriptablePackage::ScriptablePackageExtractor extractor(header);

std::uint32_t idx = 0;
for (; idx < reader.rootChunkTypes.size(); idx++)
    if (reader.rootChunkTypes[idx] == "PlayerDevelopmentSystem") break;

auto ref = MakeScriptedHandle(GetClass<"PlayerDevelopmentSystem">());
extractor.GetObjectById(ref, idx);

auto& data = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "playerData">(ref);
```

The double `ReadHeader` is not a typo and is not optional — it mirrors
`ScriptableSystemsContainer`'s own loader.

### C. Field by field

The node is hand-serialized: counts, then loops, then primitives. No shortcut exists; you
re-implement the game's read order exactly, using `ReadWriteEx` and the stream's typed helpers.
`Readers/BaseNativeReader` + `BufferCursor` exist for the sub-case where a field is an opaque
blob holding RTTI data.

Version gates matter most here — a branch you skip silently shifts every subsequent read.

## 3. Per-node recipes

### StatsSystem — strategy A

`game::StatsSystem::OnGameLoad` `0x1405564F4`

- Node name from `GetStatsSystemSaveNodeName` `0x1418C44E8` → `"StatsSystem"`.
- `saveVersion >= 0xA5`: `ReadStatsStateMapStructureHandleFromSave` `0x1414AACE8`, which is
  `ReadPackage` of `gameStatsStateMapStructure`.
- Below `0xA5` it reads a legacy `gameStatsSystemSave` object instead
  (`game_StatsSystem_LoadLegacyStatsSystemSave` `0x14252196C`). **We never see this** — the mod
  rejects pre-2.00 saves.
- The node yields parallel `keys` (`StatsObjectID`) and `values` (`SavedStatsData`) arrays; index
  them together into a hash map.

**Already implemented** in `Parsing/New/Nodes/StatsSystem.cpp`, and it is the reference example
for strategy A. See [item-quality-and-stats-load.md](item-quality-and-stats-load.md) §2 for what
the contents mean and §4.1 for why all three of `modifiersBuffer`, `forcedModifiersBuffer` and
`inactiveStats` matter.

### ScriptableSystemsContainer — strategy B

Loader `sub_140493F9C`, node-name getter `sub_140493F54` → `"ScriptableSystemsContainer"`
(`CName` hash `0x58143B8E9CF599B`).

```cpp
if (aStream.GetSaveVersion() < 3) return false;   // the game's own gate

Save::NodeAccessor node(aStream, GetName(), true, false);
auto buffer = aStream.ReadBuffer();               // BaseStream_ReadRawBuffer
// → package reader, as in strategy B
```

The save side (`sub_1425368B0`) is the exact mirror: accessor, then one raw-buffer write.

Systems inside worth extracting: `PlayerDevelopmentSystem`, `EquipmentSystem`, `CraftBook`,
`DataTrackingSystem`. V1's `ScriptableContainerNodeV2` is the list of what the mod actually needs.

### inventory + itemData — strategy C

`game::InventoryManager::OnGameLoad` `0x14099C388`. Node name from `GetInventorySaveNodeName`
`0x1403EADB8` → `"inventory"`.

This is the hard one, and the reason the PoC comment says *"once we figure out
FDB/persistency/inventory"*. Read order:

```
u32   entityInventoryCount
repeat entityInventoryCount:
    u64   ownerHash                     (sub_140692DDC)
    u32   itemCount
    repeat itemCount:
        ItemID  itemRecordID            (sub_14024B3DC)
        u32     flags/quantity          (ReadWriteEx, layout varies by save version)
        u8      itemHeaderByte0E
        ...
```

Two things make it awkward:

- **Version-dependent field sets.** The loader branches on `saveVersion >= 0xF8` for one extra
  field up front, and on `(stream[8] & 1)` for a different per-item read order.
- **`itemData` is a nested node**, not a field. Unique items carry a separate `"itemData"` node
  (`GetInventoryItemDataSaveNodeName` `0x14099C988`), read by
  `game_UniqueItemData_LoadItemDataNode` `0x14099B4D8` — which is also where each item's
  `InnerItemData` lives (`game_InnerItemData_Serialize`). V1 mirrors this by recursing with
  `ParseNode` on child nodes (`InventoryNode.hpp:190`); V2 must nest `NodeAccessor` scopes.

`Parsing/New/Nodes/Inventory.hpp` already declares the shapes to fill
(`ItemRepresentation`, `InnerItemDataRepresentation`, `ItemLootData`, and the three
`*ItemDataRepresentation` variants matching `gameUniqueItemData` /
`gameBlueprintStackableItemData` / `gameStackedItemData`).

> If `InnerItemData` is ever wanted — it is the unexplored candidate for clothing `+` tiers, see
> [item-quality-and-stats-load.md](item-quality-and-stats-load.md) §5.2 — this is the node that
> carries it, and neither V1 nor V2 reads it today.

### FactsDB / FactsTable — strategy C

`quest::QuestsSystem::OnGameLoad` `0x1407C583C`. Name getters: `sub_1407C703C` → `"FactsDB"`,
`sub_1407C5DE4` → `"FactsTable"`.

The quest system opens one accessor and then runs a fixed chain of sub-readers, each taking the
stream and returning bool:

```
sub_1407C6474   (this, stream)
vfunc +80 on QuestsSystem+0xF8
saveVersion >= 0x8B ? sub_1407C6440  (quests list)
                    : legacy CString-array path → ResourcePath
sub_1407C6390 / sub_1407C62E4 / sub_1407C61AC / sub_1407C5FEC
vfunc +152 / sub_1407C5F40 / vfunc +56 / sub_1403AAEC4
→ job::Builder + JobQueue_SyncWait
```

For NGP+ only the facts matter, so this can be read far more narrowly than the game does — but
the fields still have to be consumed **in order** to stay aligned.

### PersistencySystem2 — strategy C, two nodes

`game::PersistencySystem::OnGameLoad` `0x140249E80`. Name getter `sub_1418C4914` →
`"PersistencySystem2"`.

- Gated on `saveVersion >= 205` (`0xCD`).
- Opens an **optional header node first** (`sub_1418C48CC`, accessor args `(0, 0)`), reads a
  version triple, and compares against the live registry. Mismatch is a hard fail path.
- Then opens `"PersistencySystem2"` proper with `(1, 0)` under a `scoped_lock`.

V1 narrows this aggressively — `PersistencySystemNode.hpp:111` has
`if constexpr (m_onlyDoVehicleGarage)`. Keep that narrowing in V2; do not try to read the whole
node.

### WardrobeSystem

`"WardrobeSystem"` appears at `sub_14102429C` and `sub_1412E532C`, but the latter is the RTTI
class registration, not a loader — **I did not locate its `OnGameLoad`**, so its strategy is
unconfirmed. V1's `WardrobeSystemNode` is the reference until someone traces it.

## 4. Version gates seen

| node | gate | note |
| --- | --- | --- |
| StatsSystem | `>= 0xA5` | else legacy `gameStatsSystemSave` |
| StatsSystem | `< 0xE2` | `FilterDeprecatedVitalsModifiers` fixup |
| ScriptableSystemsContainer | `< 3` | skip entirely |
| inventory | `>= 0xF8` | one extra leading field |
| inventory / itemData | `>= 0xE3`, `>= 0xDD`, `>= 0xBE`, `>= 0x61`, `>= 0x55`, `>= 0x33` | per-field |
| QuestsSystem | `>= 0x8B` | else legacy quest-list path |
| PersistencySystem | `>= 205` | else nothing is read |

**Save version and game version are different counters.** The save header
(`Parsing/Definitions/FileInfo.hpp`) carries both:

| field | value on a current save | note |
| --- | --- | --- |
| `saveVersion` | `269` (`0x10D`) | what every gate above compares against |
| `gameVersion` | `2310` | flat build number, not packed, not semver |

`MinSupportedGameVersion = 2000` in `Filesystem/SaveFS.cpp` gates on **gameVersion**, and the code
says so itself: *"Should be using saveVersion instead, but I don't know the proper save version
for 2.00"*.

The two counters are asymmetric, which is worth knowing before hunting for a constant that does
not exist:

- **gameVersion is an RTTI-reflected enum.** `RED4ext::game::GameVersion` names every build, and
  carries a `Current` member updated per release:

  ```
  CP77_Patch_2_0 = 2000,   // == MinSupportedGameVersion
  CP77_Patch_2_1 = 2100,
  CP77_Patch_2_2 = 2200,
  CP77_Patch_2_3 = 2300,
  Current        = 2310,   // this build, and what current saves carry
  ```

  So `MinSupportedGameVersion` could be `game::GameVersion::CP77_Patch_2_0` rather than a magic
  number, and `GameVersion::Current` is available if "save from exactly this build" is ever needed.

- **saveVersion has no enum.** Nothing in the RTTI dump contains 269, the IDB has no such local
  type, and no symbol mentions it. It exists only as immediates in the decompiled gates. If the
  game has an internal C++ enum for it, it is unreflected and stripped, so it cannot be recovered
  by name — only the values are observable.

Two consequences:

- On saves at `saveVersion 269` every gate in the table is **below** the save's version, so all of
  them are unconditionally taken. None of them branch.
- Whether any gate branches across the *accepted* range depends on the `saveVersion` of a
  gameVersion-2000 save, which is **not established**. Reading the header of a genuine 2.00-era
  save would settle it; until then, do not assume a gate is dead just because current saves clear
  it.

## 5. Gotchas

- **Check `IsGood()` after every step.** The stream latches failure; a missed check turns into
  garbage reads rather than an error.
- **`NodeAccessor` is scoped, and nesting is real.** `GameSessionDesc` → `game::SessionConfig` in
  the PoC is a nested pair. Reads must happen inside the scope.
- **Node names come from the name pool.** Use `NamePoolRegistrar<"literal">::Get()`. A wrong name
  fails *silently* by resolving to the wrong node — that was a live bug in
  `Parsing/New/Nodes/StatsSystem.cpp`, which returned `"ScriptableSystemsContainer"`.
- **`ReadPackage` gives you `Handle<ISerializable>`.** The `reinterpret_cast` to the concrete type
  is expected; the handle owns the memory, so keep it alive as long as you hold pointers into it
  (`StatsSystemNode` keeps `m_handle` alive for exactly this reason).
- **Strategy C is order-sensitive.** Skipping a version-gated field shifts every later read. When
  a node comes out wrong, suspect a missed gate before suspecting the field types.
- **Don't fan out reads across threads.** V1 parses root nodes on a `JobQueue`; V2 reads through
  one stream with a cursor, so node reads are inherently sequential.
