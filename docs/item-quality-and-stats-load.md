# Item quality, the stats system, and save load

Reverse-engineering notes for Cyberpunk 2077 (`Cyberpunk2077.exe`, image base `0x140000000`).
Addresses below are VAs; subtract `0x140000000` for RVAs suitable for the
`cp2077-address-hash` lookup → `RED4ext::UniversalRelocFunc`.

Confidence: binary-confirmed unless noted.

---

## 1. Quality is a float stat, not an enum

`gamedataStatType.Quality` = **1171**. `gamedataStatType.PowerLevel` = **1146**.

The stat holds a float, normally `0.0 .. 4.0`. The `gamedataQuality` enum value is *derived*
from it, and the enum's own numbering is not the stat scale:

| stat value | `gamedataQuality` | enum int |
| --- | --- | --- |
| 0 | `Common` | 0 |
| 1 | `Uncommon` | 11 |
| 2 | `Rare` | 9 |
| 3 | `Epic` | 2 |
| 4 | `Legendary` | 5 |
| anything else | `Common` | 0 |

In 2.x the *displayed* tier is `Quality * 2 + IsItemPlus` — see `PlayerPuppet.RetroFixItemQuality`
(`player.swift:~2429`) and `RPGManager.ForceItemTier`. So preserving the visible tier needs
**both** `Quality` and `IsItemPlus`. `WasItemUpgraded` feeds `RPGManager.GetItemTierForUpgrades`,
a separate 0..10 scale.

### Script side

`RPGManager.GetItemQuality(qualityStat: Float)` (`rpgManager.swift:612`) does `RoundF(qualityStat)`
then switches 0..4.

### Native side — exactly one implementation

`sub_140383518` @ **`0x140383518`** is the only native float → `gamedataQuality` mapper:

```c
__int64 __fastcall sub_140383518(float a1)   // cvttss2si, then switch
{
  switch ((int)a1) { case 0: return 0;  case 1: return 11;
                     case 2: return 9;  case 3: return 2; }
  return (int)a1 != 4 ? 0 : 5;
}
```

It has three xrefs, all `RPGManager` script natives:

| script native | thunk | notes |
| --- | --- | --- |
| `GetFloatItemQuality(Float)` | `0x1414554D4` | direct |
| `GetItemDataQuality(wref<gameItemData>)` | `0x140383430` → `0x1403834A4` | reads stat 1171; returns `Invalid` (14) on a dead handle |
| `GetInnerItemDataQuality(InnerItemData)` | `0x1403832D8` | reads stat 1171 via `0x140383658` |

> **Mismatch worth knowing:** the native uses `cvttss2si` — truncation toward zero. The REDscript
> path uses `RoundF` — rounding. A `Quality` stat of `1.6` is `Uncommon` via
> `RPGManager.GetItemDataQuality` but `Rare` via
> `RPGManager.GetItemQuality(itemData.GetStatValueByType(...))`. Keep quality values integral.

### Other native readers of stat 1171 (none re-do the mapping)

Found by scanning every `mov r32, 493h` site in the image. All consume the raw float:

- `0x14065C848` — loot-container "highest quality" tracking; inline `(int)quality`, and
  `IsItemIconic` (889) `> 0` short-circuits to `5`.
- `0x14288D864` — item sort comparator; packs `quality * 16777215` into a sort key.
- `0x140C1B4C4` — inventory/UI grouping key; `quality * 1000`.
- `0x1407FC64C` — copies stats 1171 and 890 between objects.
- `0x14068613C` — the `Quality.Random` roll at item creation (see §3).

So: one canonical mapper, and it is only reachable from script.

---

## 2. How quality is restored on game load

Quality persists as a serialized `gameConstantStatModifier` (statType 1171) inside
`SavedStatsData::modifiersBuffer`, keyed by the item's `StatsObjectID`.

### 2.1 `game::StatsSystem::OnGameLoad` — `0x1405564F4`

Save version `>= 0xA5`: opens the `StatsSystem` node, reads a `gameStatsStateMapStructure`
(`ReadStatsStateMapStructureHandleFromSave` `0x1414AACE8`), and for each entry:

1. `game_SavedStatsData_DeduplicateType888Modifiers` `0x140556754` — keeps only the first
   `IsItemCrafted` (888) modifier in the legacy `statModifiers` array.
2. `game_StatsStateMap_Insert` `0x141CB9450` → a **pending map at `statsSystem + 928`**.

Nothing is applied yet. Then `RemapLegacySerializedStatTypes` `0x141CB9A00`, and for version
`< 0xE2`, `FilterDeprecatedVitalsModifiers` `0x141CB9780`. Version `< 0xA5` goes through
`LoadLegacyStatsSystemSave` `0x14252196C` instead.

### 2.2 `RebuildRuntimeStatsAfterLoad` — `0x140997DD0`

Walks stats objects that **already exist**, finds their saved data, applies it, erases the entry.
Items do not exist yet at this point, so this covers the player and world objects.

### 2.3 Per-object consumer — `StatsSystem` vtable **+536** = `sub_141CBA160` `0x141CBA160`

(vtable at `0x142C9B3B8`; slot 0 is `gameStatsSystem::GetType`.)

Given a `StatsObjectID`, finds the pending `SavedStatsData`, builds a `StatsBundle`, and replays
it. Field offsets match `RED4ext::game::SavedStatsData` exactly:

| `SavedStatsData` field | offset | applied by |
| --- | --- | --- |
| `statModifiers` (legacy array) | `0x00` | `sub_140843268` |
| **`modifiersBuffer`** | `0x10` | `sub_14099875C` → RTTI `CreateObject` + `Serialize(flags=269)` → `StatsObject::AddSavedModifierKey` `0x1409988E4` — **quality lands here** |
| `forcedModifiersBuffer` | `0x38` | `sub_140998074` → `sub_14099725C` |
| `savedModifierGroupStatTypesBuffer` | `0x60` | `sub_142521AAC` / `sub_14252523C` |
| `inactiveStats` | `0x88` | `sub_140997A50(obj, statType, 0)` |
| `recordID` / `seed` | `0x98` / `0xA0` | `sub_1411330F4` |

Then it erases the pending entry.

### 2.4 How an item gets there — `ItemData` vtable **+344** `RebindLoadedStatsObject`

Pure virtual on `gameItemData`. Implementations:

- `game_ItemData_RebindLoadedStatsObject` `0x141CB4670` — `gameUniqueItemData`,
  `gameBlueprintStackableItemData`, `gamePreviewItemData`
- `sub_141CBABB0` `0x141CBABB0` — `gameStackedItemData`

It computes `ItemID::ToStatsObjectID` `0x1405E9D9C`, calls `StatsSystem` vtable+536 first, and
**only if there is no saved entry** falls through to `StatsBundle::InitializeStats` `0x140654334`
— the fresh path that rolls quality from the record.

Entry point during load, `game_UniqueItemData_LoadItemDataNode` `0x14099B4D8`:

```c
(*(this->vtable + 344))(this, gameInstance, ownerStatsObjectID, 0, saveVersion >= 0xE3);
```

Later, `TransactionSystem::OnPlayerAttachedCallback` → `Inventory::ReinitializeAndReattachStatsOnItems`
`0x140383B54` re-runs `ReinitializePlayerStats` (item vtable+416, `0x140383D94`) over the inventory
to rebind stat relations.

### 2.5 Save side (mirror)

`StatsSystem` vtable+304 `sub_142521D10` → `sub_142521D9C` `0x142521D9C`:

1. **Re-emits every still-pending, never-consumed entry** from `statsSystem+928` into the new
   structure. Saved stats for items that were never instantiated survive round-trips indefinitely.
2. Walks live stats objects; for each, `sub_142521740` `0x142521740` with three memory streams
   (0x4000 / 0x2000 / 0x2000) which become `modifiersBuffer` / `forcedModifiersBuffer` /
   `savedModifierGroupStatTypesBuffer`.
3. `sub_1425226C0` `0x1425226C0` (the `modifiersBuffer` writer) enumerates
   `statsObject->statsObjectData(+352)->savedModifierStore(+80)` and serializes each entry.

`ItemID::ToStatsObjectID` `0x1405E9D9C` is byte-for-byte identical to this repo's
`modsave::StatsSystemNode::GetEntityHashFromItemId` — same `0xC6A4A7935BD1E995` /
`0x35A98F4D286A90B9` constants, same `uniqueCounter == 0` branch, same final
`((tmp * c) ^ tdbid) * c`.

### 2.6 `modifiersBuffer` element format

Each element is `CName className` then the modifier's own `Serialize(stream, version)`. Both
the reader (`sub_14099875C`) and the writer (`sub_1425226C0`) pass a **hardcoded version of
269** (`0x10D`), independent of the save version — so the element layout is fixed for a given
game build.

`gameConstantStatModifier::Serialize` `0x140998CAC`, `gameCurveStatModifier::Serialize`
`0x1409970B4`, `gameCombinedStatModifier::Serialize` `0x142529858` all branch on that version:

- `statType`: **2 bytes** (raw u16 enum index) when `version < 0xD1`, else an **8-byte CName
  hash** resolved through the RTTI enum.
- curve `curveStat` / combined `refStatType`: **4 bytes** when `version < 0xD3`, else an
  **8-byte CName hash**.

At 269 both take the CName-hash path, which is what
`modsave::GetStatModifiersInternal` implements. Buffers written by a pre-2.0 build use the
narrow encodings and would misparse, but NGP+ rejects those saves outright
(`MinSupportedGameVersion = 2000` in `src/Filesystem/SaveFS.cpp:80`), so no legacy handling is
needed. The game's own fixup for them is
`game_StatsStateMap_RemapLegacySerializedStatTypes` `0x141CB9A00`.

---

## 3. Where quality is first created

`sub_14068613C` `0x14068613C`, at item creation:

- If the item record's `Quality` FK is `Quality.Random`, it reads the current stat 1171 as an
  upper bound, PCG-rolls `[0, bound]`, removes existing 1171 modifiers, then
  `BuildSavedModifierKey` + `StatsObject::AddSavedModifierKey` with the rolled value.
- Same shape for `PowerLevel` (1146).

Because it goes through `AddSavedModifierKey` → `SavedModifierStore`, the roll is exactly what
`modifiersBuffer` serializes. That is why quality survives without any dedicated item field.

---

## 4. Consequences for NGP+

### 4.1 Saved modifiers are deltas over the record base — do not clear first

This is the single most important thing on this page, and it is easy to get backwards.

Re-read §2.3. When the game restores an item, `sub_141CBA160` does, in order:

1. `StatsBundle::MakeHandle` — a **fresh** bundle
2. `sub_1411330F4(bundle, …, savedStatsData->recordID, &savedStatsData->seed, …)` — initialise
   it from the **item record and the saved RNG seed**
3. `sub_14099875C` — apply `modifiersBuffer` **on top**, removing nothing
4. `inactiveStats` — the *only* removal, an explicit per-stat suppression list

So the buffer holds **deltas layered over a record-driven base**, not a self-contained
description of the item. For an item whose record has a fixed `Quality`, the base is not in the
buffer at all — only the deltas (the retrofix's negative `Quality` term, upgrade counts, …) are.

`TransactionSystem.GiveItem` / `GiveItemByItemData` reproduces step 2, and because `ItemID`
carries its `rngSeed`, a random-quality item re-rolls to the same value it originally had. So
after `GiveItem` the item already holds the correct base, and the saved modifiers should simply
be added on top — exactly as the game itself would.

Calling `RemoveAllModifiers` first strips that base and leaves only the deltas. For a
retrofixed item that means the negative `Quality` term survives alone and **every item lands on
Tier 1**. Observed in testing; do not reintroduce it.

If you ever do need to suppress a record-driven modifier, `inactiveStats`
(`SavedStatsData +0x88`, already parsed as `StatsSystemNode::GetDisabledModifiers`) is the
channel the game uses. It is not currently exposed on `NGPlusItemData`.

### 4.2 …but vanilla's *runtime* forcing recipe is remove-then-add

Do not confuse the two. Sites that **assert** a quality at runtime do remove-then-add:

```swift
SS.RemoveAllModifiers(itemData.GetStatsObjectID(), gamedataStatType.Quality, true);
let mod = RPGManager.CreateStatModifier(gamedataStatType.Quality, gameStatModifierType.Additive, quality);
SS.AddSavedModifier(itemData.GetStatsObjectID(), mod);
```

See `RPGManager.SetDroppedWeaponQuality` (`rpgManager.swift:763`), `RPGManager.ForceItemTier`
(`:787`), `PlayerPuppet.RetroFixItemQuality` (`player.swift:2430`),
`PlayerPuppet.RetroRescaleNonIconicWeapons` (`:2485`).

That pattern is correct when you are overwriting a value you computed yourself. It is **wrong**
for replaying a save, which is §4.1's job. NGP+ is replaying a save.

`RemoveAllModifiers` does also release saved-modifier keys (`sub_140997A50` →
`sub_140654ACC`), which add-on-top does not — but restricting *which* stats get carried at all
(§4.5) is the right lever for the key pressure in §4.3, not blanket clearing.

### 4.3 Likely cause of the crashes: saved-modifier key exhaustion

Saved modifiers are interned globally in `StatsRegistry` as **16-bit handles** into a fixed table
at `registry + 96 .. registry + 524376` — exactly **65535 slots**. `0xFFFF` is the invalid sentinel.

`game::StatsRegistry::FindOrCreateSavedModifierKey` `0x1404539F0` dedupes by hash, so identical
`(statType, modifierType, value)` triples share one key and just refcount — but it skips dedup
entirely for `Random`-type modifier descriptors, and per-item random floats (`RandomCurveInput`,
`NPCWeaponDropRandomizer`, `LootLevel`, …) are unique by construction.

`sub_14045594C` `0x14045594C` is a free-list pop **with no exhaustion check**. When the list is
empty it hands back the `0xFFFF` sentinel; `CreateSavedModifierKey` `0x1404556E8` also explicitly
stores `0xFFFF` when `sub_141CCA580` fails.

The consumer does not guard it:

```text
game::StatsSystem::AddSavedModifierByObjectID      0x140997428
  -> game::StatsObject::AddSavedModifierKey        0x1409988E4
     -> SavedModifierStore::TryAddSavedModifierKey 0x1406553D4
        -> ValidateGroupStatType 0x140291564       // only checks groupStatType != 1710
        -> SavedModifierStore::AddSavedModifierKey 0x14065540C
             if (handle == 0xFFFF) descriptor = 0;
             (*(*(QWORD*)descriptor + 88))(descriptor);   // <-- null deref
```

So an invalid/exhausted key reaching the script-facing `StatsSystem.AddSavedModifier` is an
unguarded null-pointer dereference. Re-applying every modifier of every carried item — without
removing the record-driven ones first — is precisely the workload that gets there.

### 4.4 Why "just read the Quality modifiers" does not work

`Quality` is not self-contained. It sits in a small dependency cluster:

| stat | id |
| --- | --- |
| `Quality` | 1171 |
| `WasItemUpgraded` | 1669 |
| `IsItemPlus` | 890 |
| `ForceQualityHelper` | 644 |
| `QualityToMaxQualityRatio` | 1173 |
| `CommonTierFailsafe` | 441 |
| `EffectiveQualityToMaxQualityRatio` | 579 |

Three things break naive reconstruction:

**(a) The 2.0 iconic retrofix deliberately cancels `Quality` out.**
`PlayerPuppet.SetIconicWeaponsTier` (`player.swift:2644`),
`PlayerPuppet.RescaleOwnedIconicsToPlayerLevel` (`:2687`) and the stash mirrors
(`stash.swift:500`, `:796`) write *snapshot* constants in a strict order, each reading
`GetStatValueByType` at creation time:

```swift
WasItemUpgraded += Quality * 2.0            // reads Quality now
Quality         += WasItemUpgraded * -0.5   // reads WasItemUpgraded AFTER the line above
WasItemUpgraded += IsItemPlus
RemoveAllModifiers(IsItemPlus)
IsItemPlus      += curve(WasItemUpgraded, "quality_curves", "iconic_upgrades_amount_to_plus")
```

`SetIconicWeaponsTier` only fires when `WasItemUpgraded < 1.0`, so at line 2
`WasItemUpgraded == 2Q` and the modifier written is `-0.5 * 2Q == -Q`. **Net effective
`Quality` becomes 0**, and the real tier now lives in `WasItemUpgraded` (on the 0..10
`RPGManager.GetItemTierForUpgrades` scale) plus a curve-derived `IsItemPlus`.

So for a retrofixed iconic, summing the Additive constant `Quality` modifiers yields `0` —
Common. Taking the absolute value instead yields `Q + |−Q| = 2Q`, which is the
`WasItemUpgraded` / `EffectiveTier` scale, not the `Quality` scale. Neither is "the quality";
which one is useful depends on what you feed it back into.

**(b) Some `Quality` modifiers are curve modifiers, not constants.**
`RPGManager.ForceItemTier` (`rpgManager.swift:783`) writes:

```swift
ForceQualityHelper += value                                              // constant
Quality            += curve(ForceQualityHelper, "quality_curves", "iconic_upgrades_amount_to_quality")
IsItemPlus         += curve(ForceQualityHelper, "quality_curves", "iconic_upgrades_amount_to_plus")
```

The contributed value is **not in the buffer** — it is evaluated lazily from the curve against
whatever `ForceQualityHelper` currently is. `ApplyStatModifiers` filters `Quality` out of its
generic re-apply loop, so any curve or combined `Quality` modifier is **silently dropped**.

**(c) `WasItemUpgraded` accumulates.**
`CraftingSystem.UpgradeItem` (`craftingSystem.swift:1021`) is well-behaved —
`RemoveAllModifiers(WasItemUpgraded)` then one constant. But every retrofix path *adds without
removing*, and `RescaleOwnedIconicsToPlayerLevel` even adds a `WasItemUpgraded * -1.0` zeroing
modifier before re-deriving. A long-lived save therefore has a stack of partially cancelling
`WasItemUpgraded` constants whose order of application matters.

This is why the accumulation loop in `ApplyStatModifiers` exists. It is not incidental.

### 4.5 Recommended shape

Reconstructing a single quality number offline means re-implementing that graph, including
curve evaluation against `quality_curves`. Two workable options:

**Option A — replay the whole buffer verbatim (implemented).** Do not collapse anything to one
number, do not clear, and **do not filter by stat type**. Re-apply every saved modifier in its
original order with its original kind (constant / combined / curve), on top of the base
`GiveItem` already established (§4.1). Curve modifiers stay curve modifiers and re-derive
against whatever the earlier ones left behind.

Filtering to the §4.4 cluster looks reasonable and is not. Measured on a test save:

| item | before | after | `IsItemPlus` | `Quality` |
| --- | --- | --- | --- | --- |
| Erebus | t5++ | t5++ ✓ | 2 ✓ | 4 ✓ |
| Majesty | t5++ | t1++ | 2 ✓ | 0 ✗ |
| Mal | t5+ | t1+ | 1 ✓ | 0 ✗ |
| Crusher | t5+ | t1+ | 1 ✓ | 0 ✗ |
| Fang | t5+ | t1+ | 1 ✓ | 0 ✗ |
| Errata | t5+ | t1+ | 1 ✓ | 0 ✗ |

The `+` suffix survived everywhere, so `IsItemPlus` and its curve driver were being restored
correctly. Only the base `Quality` was lost — on five items out of six.

**Why Erebus survived is not established.** It is not a fixed-max-tier item, so the obvious
explanation does not hold, and the difference has not been traced to a specific stat. What the
result does establish is that the whitelist is lossy: some of the `Quality` contribution for
these items rides on stats outside the §4.4 cluster. Plausible carriers are `EffectiveTier` and
`QualityToMaxQualityRatio`, since `UnifyIconicsUpgradeCountWithEffectiveTier`
(`player.swift:2763`) drives `WasItemUpgraded` off `EffectiveTier` and `ForceItemTier` clears
`QualityToMaxQualityRatio` alongside `Quality` — but that is inference, not measurement.

Rather than keep guessing at the boundary of the graph, replay all of it, as the game does. If a
narrower set is ever wanted, dump the buffer for Erebus and Majesty side by side
(`StatsSystemNode::DumpStatModifiersToConsole`) and let the diff decide it.

**Option B — evaluate offline.** All inputs are available: constants from `modifiersBuffer`,
curves from TweakDB `quality_curves`. Compute the effective `Quality` and `IsItemPlus`, then
remove-then-add one constant each. Only viable if you also reproduce the record base, since the
buffer alone does not contain it.

Either way:

- Mirror the load path: record base first, saved deltas on top, no clearing. See §4.1.
- If nothing was saved for a stat, leave it alone; do not force `0.0`.
- Check `AddSavedModifier`'s `Bool` return, as vanilla does at `player.swift:2434`.
- `ScalingBlocked` stays add-only, matching `PlayerPuppet.BlockScaling` — a record-driven block
  should not be cleared, and stacking to `2.0` still reads as blocked.
- If you go with Option B, round the final `Quality` to an integer — see the truncate/round
  mismatch in §1.

---

## 5. The stash retrofix chain (measured root cause)

Transferred stash items dropped to Tier 1 while inventory items came through correctly. The
cause is not in what NGP+ carries — it is that the game re-runs its 2.x migrations over them.

`Stash.OnOpenStash` calls `Stash.ProcessStashRetroFixes`, which is a chain of ~17 one-shot
migrations, each gated only by its own fact:

```swift
factVal = GetFact(game, n"IconicReworkCompletedInStash");
if factVal <= 0 && true {
    Stash.IconicsReworkCompensateInStash(stashObj);
    SetFactValue(game, n"IconicReworkCompletedInStash", 1);
};
```

A fresh NG+ game has every one of those facts at 0, so the whole chain fires the first time the
player opens the stash — over items that came from an already-post-2.0 save.

The destructive one is `Stash.RescaleStashedIconicsToPlayerLevel` (`stash.swift:491`):

```swift
zeroUpgradeMod = CreateStatModifier(WasItemUpgraded, Additive, GetStatValueByType(WasItemUpgraded) * -1.00);
...
qualityToUpgradeMod = WasItemUpgraded += Quality * 2.00;
upgradeToQualityMod = Quality         += WasItemUpgraded * -0.50;
RemoveAllModifiers(IsItemPlus);
upgradeToPlusMod    = IsItemPlus       = curve(WasItemUpgraded);
```

It zeroes `WasItemUpgraded` and rebuilds it from `Quality * 2`. Combined with §5.1 below, `Quality`
reads 0 at that moment, so the item collapses to `Quality 0 / IsItemPlus 0 / WasItemUpgraded 0`.
`UnifyIconicsUpgradeCountWithEffectiveTierInStash` and `ProcessNonIconicWeaponsRescaleInStash`
rewrite the same stats off the same trigger — the latter is why a non-iconic like Crusher was
affected too.

The unconditional path, `Stash.ScaleStashIconicsToPlayerLevel`, is already harmless: it skips
items with `ScalingBlocked >= 1`, which `ApplyStatModifiers` sets.

Fix: mark the gates done in `LoadFacts`, the same statement `q000_patch_2_0_new_game` already
makes for the player side. `wat_sts_counter` and `regina_iconic_subdermalcoprocessor_acquired`
are left alone — gameplay state, not migration gates.

### 5.1 Stash item stats are lazily initialised

This confounded the whole investigation and is worth stating on its own. **A stash item's stats
read 0 until the stash is actually opened.** Same save, same item, one log:

```
(console-stash)  Quality=0                    ← before opening the stash
(console-stash)  Quality=4.000000/4.000000    ← after opening it
```

So any measurement of stash items taken at transfer time, or before the player opens the stash,
is meaningless. Two conclusions in this document's history were drawn from exactly that artifact
and had to be retracted.

Two further notes on measuring:

- `itemData.GetStatValueByType(...)` (the item's own `StatsBundle`, what `RPGManager.GetItemQuality`
  and therefore the UI use) and `statsSystem.GetStatValue(objId, ...)` are separate native paths —
  `GetItemDataQuality` `0x1403834A4` goes through the bundle and never touches the system's map.
  In practice they agreed everywhere once initialised, but print both when in doubt.
- The displayed tier is `UIItemsHelper.GetQualityF(qualityInt, isIconic, plusValue)` =
  `qualityInt + (isIconic ? 0.05 : 0) + plusValue * 0.10`, with `QualityToInt(Legendary) = 8`. So
  `8.05` is T5, `8.15` is T5+, `8.25` is T5++, and `0.05` is T1 on an iconic. Reading that number
  directly beats inferring tiers from stat values.
