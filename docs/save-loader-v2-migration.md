# Migrating the save loader to the native stream

Notes for replacing the hand-rolled ("WKit-style") save parser with the game's own save stream.
The native path already exists in the tree in two places — a proof of concept behind an
`if constexpr`, and a partly-built `Parsing/New` node tree — but nothing on the load path uses it.

Status as written: **V1 is live, V2 is unreferenced.**

---

## 1. What runs today (V1)

```
mod::NewGamePlusSystem::LoadSaveData(saveName)         GameSystem/NewGamePlusSystem.cpp:279
  └─ parser::Parser::ParseSavegame(saveName)           Parsing/FileReader.hpp
       ├─ read whole file into std::vector<std::byte>
       ├─ DecompressFile()  → m_decompressedDataRaw
       ├─ build m_flatNodes / m_nodeList from the node table
       ├─ FindChildren + CalculateTrueSizes
       └─ LoadNodes()
            └─ per root node, on a JobQueue (1000 ms timeout):
                 modsave::ParseNode(cursorCopy, node)  Parsing/.../ParserHelper.cpp:61
                   └─ FindParser(node.m_hash) → NodeType::ReadData(FileCursor&, NodeEntry&)
  └─ NGPlusProgressionData(parser)                     GameSystem/Definitions/PlayerSaveData.cpp
       └─ parser.LookupNodeData<modsave::XNode>() per node
            └─ GameSystem/DataReaders/*Reader → RTTI result objects for script
```

Dispatch is a macro chain in `ParserHelper.cpp`, keyed on `NodeType::m_nodeName`:

| V1 node class | node name |
| --- | --- |
| `InventoryNode` | `inventory` |
| `ItemDataNode` | `itemData` |
| `ScriptableSystemsContainerNodeV2` | `ScriptableSystemsContainer` |
| `PersistencySystemNode` | `PersistencySystem2` |
| `FactsDBNode` | `FactsDB` |
| `FactsTableNode` | `FactsTable` |
| `StatsSystemNode` | `StatsSystem` |
| `WardrobeSystemNode` | `WardrobeSystem` |

Anything else falls back to `DefaultNodeData`, which only recurses into children.

Every node reads bytes by hand through `FileCursor`. That is the part V2 removes.

## 2. What the native path looks like (V2)

### 2.1 The proof of concept

`src/Filesystem/SaveFS.cpp:692`, inside `ReadSaveFileToBuffer`:

```cpp
// Tests look OK, can move to this from WKit way once we figure out FDB/persistency/inventory
constexpr auto TestSaveStream = false;

if constexpr (TestSaveStream)
```

It sets up the stream exactly once and then demonstrates three different read strategies:

```cpp
LoadSaveMetadata(GetRedPathToSaveFile(name, c_metadataFileName), metadata);
auto saveStream = RedFileManager::GetInstance()->OpenBufferedFileStream(
                      GetRedPathToSaveFile(name, c_saveFileName));
auto loadStream = Save::Stream::LoadStream::Create(saveStream, metadata);
loadStream.Initialize();
```

| node | strategy |
| --- | --- |
| `GameSessionDesc` → `game::SessionConfig` | `loadStream->ReadWriteEx(&path)` for raw fields |
| `ScriptableSystemsContainer` | `loadStream.ReadBuffer()` → `ScriptablePackageReader` → `ScriptablePackageExtractor::GetObjectById` |
| `StatsSystem` | `loadStream.ReadPackage(GetClass<game::StatsStateMapStructure>())` — straight to an RTTI handle |

Node scoping is RAII:

```cpp
shared::raw::Save::NodeAccessor node(loadStream, "StatsSystem", true, false);
if (node.IsGood()) { … }
```

Two quirks the PoC already documents, both worth carrying forward:

- `ScriptablePackageReader::ReadHeader` is called **twice** — *"The game does this, don't ask"*.
- `ReadPackage` hands back a `Handle<ISerializable>` whose `instance` needs a `reinterpret_cast`
  to the concrete type, *"It is what it is, should fix it sometime"*.

### 2.2 The skeleton

`src/Parsing/New/` holds the beginnings of the real thing:

- **`ParserV2.hpp/cpp`** — `LoadStreamContainer::Setup(name)` does the metadata + stream +
  `LoadStream::Create` + `Initialize()` dance and is **complete**. `class ParserV2` holds one and
  is otherwise **empty** — no node registry, no dispatch, no lookup.
- **`TypeDefinitions/NGPlusSaveNode`** — `parser::node::SaveNodeData : Red::ISerializable`, with
  `virtual bool OnRead(LoadStream&)` and `virtual CName GetName()`. RTTI-registered as abstract.
  This is the V2 equivalent of `modsave::NodeDataInterface`.
- **`Readers/BaseNativeReader` + `BufferCursor`** — a generic RTTI-driven blob reader
  (`ReadClass`, `ReadHandle`, `ReadProperty`, `ReadArray`, `ReadEnum`, `ReadDataBuffer`,
  `ReadTDBID`, `ReadCName`, `ReadNodeRef`).

  **Treat this as a fallback, not a design goal.** It is itself a re-implementation of decoding
  the game already does, which is the exact thing V2 exists to stop doing. Reach for a game
  reader first — see the ladder in
  [native-save-reading.md](native-save-reading.md) §2.4 — and only fall back here when no game
  reader covers the shape.

### 2.3 Ported node status

| V2 node | state |
| --- | --- |
| `Nodes/StatsSystem` | **Working.** `NodeAccessor` → `ReadPackage(GetClass<game::StatsStateMapStructure>())` → `tsl::hopscotch_map<uint64_t, SavedStatsData*>`. Structurally identical to V1's map, minus all the manual cursor work. |
| `Nodes/ScriptableSystemsContainer` | Package-reader path, double `ReadHeader` as in the PoC. |
| `Nodes/Inventory` | **Stub.** `OnRead` opens the `NodeAccessor` and returns. The representation structs (`ItemRepresentation`, `InnerItemDataRepresentation`, `UniqueItemDataRepresentation`, `BlueprintStackableItemDataRepresentation`, `StackedItemDataRepresentation`, `ItemLootData`) are declared; only `InnerItemDataRepresentation` and `ItemLootData` have fields. |

> **Bug to fix before wiring anything up.** `Parsing/New/Nodes/StatsSystem.cpp` returns the wrong
> node name:
>
> ```cpp
> CName StatsSystemNode::GetName() noexcept
> {
>     return NamePoolRegistrar<"ScriptableSystemsContainer">::Get();   // should be "StatsSystem"
> }
> ```
>
> It reads the wrong node today. It has never been noticed because nothing calls it.

## 3. Why bother

- **Deletes the format guesswork.** V1 re-implements node discovery, child linking, true-size
  calculation, decompression and per-field byte layout. All of that is the game's job in V2.
- **Version drift stops being our problem.** `docs/item-quality-and-stats-load.md` §2.6 lists
  serialization branches keyed on version (`< 0xD1`, `< 0xD3`). V1 has to track them by hand;
  `ReadPackage` does not.
- **Whole-file decompression goes away.** V1 reads and decompresses everything up front and holds
  it; the stream reads what it is asked for.
- **The 1000 ms `WaitForQueue` in `LoadNodes()` goes away** along with the job fan-out, or at
  least becomes a deliberate choice rather than a requirement of parsing everything eagerly.

## 4. Migration plan

Staged so V1 stays live throughout and each stage is independently verifiable.

### Stage 0 — make V2 addressable

1. Fix the `StatsSystem::GetName()` node name.
2. Give `ParserV2` the V1-equivalent surface: a node registry keyed on
   `SaveNodeData::GetName()`, a `ParseSavegame(name)` that runs `LoadStreamContainer::Setup` and
   then each registered node's `OnRead`, and a `LookupNodeData<T>()`.
3. Add a runtime toggle — a mod setting, not `if constexpr` — so both parsers can be run against
   the same save in one session.

### Stage 1 — StatsSystem first

It is already written, and it is the node whose output we now understand best. Run both parsers
over the same save and assert the `entityHash → SavedStatsData*` maps match: same key set, and
for each key the same `modifiersBuffer` / `forcedModifiersBuffer` / `inactiveStats` contents.

That is a real regression test, not a smoke test — the item-quality work depends on all three
channels being read correctly (see `docs/item-quality-and-stats-load.md` §4.1).

### Stage 2 — ScriptableSystemsContainer

Bigger surface (PlayerDevelopmentSystem, EquipmentSystem, CraftBook, …) but the PoC already
proves the package-reader path reaches a concrete object:

```cpp
extractor.GetObjectById(ref, classIndex);
auto& data = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "playerData">(ref);
```

Compare per-system against V1's `ScriptableContainerNodeV2` output.

### Stage 3 — the three the PoC comment calls out

*"once we figure out FDB/persistency/inventory"* — these are the hard ones and should be taken
last, individually:

- **inventory** — V2's node is a stub and the item representations are unfinished. V1's
  `InventoryNode` also hand-parses sub-nodes (`ParseNode` on `itemData` children,
  `InventoryNode.hpp:190`), so the parent/child relationship has to be re-expressed in stream
  terms. Note `InnerItemData` is serialized per item in the `itemData` node and V1 does not read
  it at all — see `docs/item-quality-and-stats-load.md` §5.2.
- **PersistencySystem2** — V1 has a `NativePersistencyReader` helper and an
  `m_onlyDoVehicleGarage` `if constexpr` narrowing what it reads. Establish whether the stream
  exposes it as a package or an opaque blob; if opaque, this is where `BaseNativeReader` earns
  its place.
- **FactsDB / FactsTable** — flat and simple, but two node types sharing one parser.

### Stage 4 — retire V1

Once every consumer in `GameSystem/DataReaders/` is fed by V2, delete `Parsing/Definitions` and
`Parsing/FileReader`, and drop `FileCursor`.

## 5. Things that will bite

- **`ResultContext` is V1-typed.** It holds `modsave::StatsSystemNode*`
  (`GameSystem/DataReaders/ResultContext.hpp`), and `NGPlusProgressionData` takes a
  `parser::Parser&`. Both need to become parser-agnostic — an interface, or a template — before
  either stage can run its output through the existing readers.
- **The readers reach into node internals.** `InventoryReader::ProcessStatModifiers` calls
  `aContext.m_statsSystem->GetStatModifiers/GetForcedModifiers/GetDisabledModifiers` directly.
  V2's `StatsSystemNode` exposes the raw map instead, so either it grows the same three accessors
  or the readers move to the map.
- **Node names are `CName`s from a pool.** V2 uses `NamePoolRegistrar<"literal">::Get()`. Getting
  one wrong fails silently by reading the wrong node — exactly the live bug in §2.3.
- **`NodeAccessor` is scoped.** Construction seeks into the node, destruction leaves it. Reads
  must happen inside the scope, and nesting mirrors the save's own nesting
  (`GameSessionDesc` → `game::SessionConfig` in the PoC).
- **Save version gates still exist**, they just move. `LoadStream::GetSaveVersion()` is there when
  a node needs it; the mod only accepts 2.00+ (`MinSupportedGameVersion` in `SaveFS.cpp`), which
  removes most of the legacy branches but not all.
