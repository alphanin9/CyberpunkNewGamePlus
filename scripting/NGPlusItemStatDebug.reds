module NGPlus.Debug

// Diagnostic for item tier/quality transfer. Not part of normal operation - set Enabled() to
// false, or delete this file and its two call sites in NGPlusOnReadyForEquipment.reds, once the
// tier transfer is settled.
//
// Two views, because they answer different questions:
//   - DumpSavedModifiers: what the save parser actually handed us for an item, before anything
//     is applied. Ground truth for what is in modifiersBuffer.
//   - DumpLiveTierStats: the stats that decide the displayed tier, read back from the live
//     stats system. Run it right after applying, and again from the console later, to see
//     whether something else moves them afterwards.
//
// See docs/item-quality-and-stats-load.md.
public class NGPlusItemStatDebug {
    public final static func Enabled() -> Bool {
        return true;
    }

    public final static func StatName(statType: gamedataStatType) -> String {
        return EnumValueToString("gamedataStatType", Cast<Int64>(EnumInt(statType)));
    }

    private final static func ModifierKindName(modifierType: gameStatModifierType) -> String {
        return EnumValueToString("gameStatModifierType", Cast<Int64>(EnumInt(modifierType)));
    }

    private final static func DescribeModifier(modifier: ref<gameStatModifierData>) -> String {
        let line = NGPlusItemStatDebug.StatName(modifier.statType)
            + " "
            + NGPlusItemStatDebug.ModifierKindName(modifier.modifierType);

        let asConstant = modifier as gameConstantStatModifierData;

        if IsDefined(asConstant) {
            return line + " constant value=" + ToString(asConstant.value);
        }

        let asCombined = modifier as gameCombinedStatModifierData;

        if IsDefined(asCombined) {
            return line
                + " combined ref="
                + NGPlusItemStatDebug.StatName(asCombined.refStatType)
                + " value="
                + ToString(asCombined.value);
        }

        let asCurve = modifier as gameCurveStatModifierData;

        if IsDefined(asCurve) {
            return line
                + " curve "
                + NameToString(asCurve.curveName)
                + "/"
                + NameToString(asCurve.columnName)
                + " driver="
                + NGPlusItemStatDebug.StatName(asCurve.curveStat);
        }

        return line + " <unrecognised modifier kind>";
    }

    public final static func DumpSavedModifiers(sys: ref<NewGamePlusSystem>, item: ref<NGPlusItemData>) {
        let modifiers = item.GetStatModifiers();
        let label = TDBID.ToStringDEBUG(ItemID.GetTDBID(item.GetItemId()));

        sys.Spew("[tier] " + label + " saved modifiers: " + ToString(ArraySize(modifiers)));

        let i = 0;

        while i < ArraySize(modifiers) {
            sys
                .Spew(
                    "[tier]   ["
                    + ToString(i)
                    + "] "
                    + NGPlusItemStatDebug.DescribeModifier(modifiers[i])
                );
            i += 1;
        }

        let forced = item.GetForcedModifiers();

        sys.Spew("[tier] " + label + " forced modifiers: " + ToString(ArraySize(forced)));

        i = 0;

        while i < ArraySize(forced) {
            sys
                .Spew(
                    "[tier]   forced["
                    + ToString(i)
                    + "] "
                    + NGPlusItemStatDebug.DescribeModifier(forced[i])
                );
            i += 1;
        }

        let inactive = item.GetInactiveStats();
        let inactiveLine = "[tier] " + label + " inactive stats: " + ToString(ArraySize(inactive));

        i = 0;

        while i < ArraySize(inactive) {
            inactiveLine = inactiveLine + " " + NGPlusItemStatDebug.StatName(inactive[i]);
            i += 1;
        }

        sys.Spew(inactiveLine);
    }

    // The UI reads item quality through RPGManager.GetItemQuality, which is
    // itemData.GetStatValueByType - the item's own StatsBundle. That is a different path from
    // statsSystem.GetStatValue(objId, ...), which looks the object up in the stats system map.
    // Natively GetItemDataQuality goes through the bundle and never touches the map, so the two
    // can disagree. Print both: "bundle/system".
    public final static func DumpItemDataTierStats(
        sys: ref<NewGamePlusSystem>,
        statsSystem: ref<StatsSystem>,
        itemData: wref<gameItemData>,
        label: String,
        phase: String
    ) {
        let watched: array<gamedataStatType>;

        ArrayPush(watched, gamedataStatType.Quality);
        ArrayPush(watched, gamedataStatType.IsItemPlus);
        ArrayPush(watched, gamedataStatType.WasItemUpgraded);
        ArrayPush(watched, gamedataStatType.EffectiveTier);
        ArrayPush(watched, gamedataStatType.IsItemIconic);

        let objId = itemData.GetStatsObjectID();
        let line = "[tier] " + label + " (" + phase + ")";
        let i = 0;

        while i < ArraySize(watched) {
            line = line
                + " "
                + NGPlusItemStatDebug.StatName(watched[i])
                + "="
                + ToString(itemData.GetStatValueByType(watched[i]))
                + "/"
                + ToString(statsSystem.GetStatValue(objId, watched[i]));
            i += 1;
        }

        sys.Spew(line + " uiQuality=" + ToString(UIItemsHelper.GetQualityF(itemData)));
    }

    public final static func DumpLiveTierStats(
        sys: ref<NewGamePlusSystem>,
        statsSystem: ref<StatsSystem>,
        objId: StatsObjectID,
        label: String,
        phase: String
    ) {
        let watched: array<gamedataStatType>;

        ArrayPush(watched, gamedataStatType.Quality);
        ArrayPush(watched, gamedataStatType.IsItemPlus);
        ArrayPush(watched, gamedataStatType.WasItemUpgraded);
        ArrayPush(watched, gamedataStatType.EffectiveTier);
        ArrayPush(watched, gamedataStatType.ForceQualityHelper);
        ArrayPush(watched, gamedataStatType.QualityToMaxQualityRatio);

        let line = "[tier] " + label + " (" + phase + ")";
        let i = 0;

        while i < ArraySize(watched) {
            line = line
                + " "
                + NGPlusItemStatDebug.StatName(watched[i])
                + "="
                + ToString(statsSystem.GetStatValue(objId, watched[i]));
            i += 1;
        }

        sys.Spew(line);
    }

    // Console command. Run after the transfer has fully settled - the apply-time dump happens
    // before the game's own retrofix passes get a look at the items, so this is the one that says
    // where they actually ended up.
    public static func DumpItemTiers() {
        let gameInstance = GetGameInstance();

        let itemList: array<wref<gameItemData>>;
        let sys = GameInstance.GetNewGamePlusSystem();
        let statsSystem = GameInstance.GetStatsSystem(gameInstance);

        GameInstance.GetTransactionSystem(gameInstance).GetItemList(GetPlayer(gameInstance), itemList);

        for itemData in itemList {
            NGPlusItemStatDebug
                .DumpItemDataTierStats(
                    sys,
                    statsSystem,
                    itemData,
                    TDBID.ToStringDEBUG(ItemID.GetTDBID(itemData.GetID())),
                    "console"
                );
        }

        NGPlusItemStatDebug.DumpStashItemTiers(gameInstance, sys, statsSystem);
    }

    // The stash is a separate inventory owner, resolved the same way LoadPlayerStash does it,
    // and it is where every item that still transfers wrong ends up living.
    private static func DumpStashItemTiers(
        gameInstance: GameInstance,
        sys: ref<NewGamePlusSystem>,
        statsSystem: ref<StatsSystem>
    ) {
        let stashList: array<wref<gameItemData>>;
        let stashId = Cast<EntityID>(
            ResolveNodeRef(
                CreateNodeRef("#v_room_stash"),
                Cast<GlobalNodeRef>(GlobalNodeID.GetRoot())
            )
        );
        let stashEntity = GameInstance.FindEntityByID(gameInstance, stashId) as GameObject;

        if !IsDefined(stashEntity) {
            sys.Spew("[tier] stash entity not resolvable, skipping stash dump");
            return;
        }

        GameInstance.GetTransactionSystem(gameInstance).GetItemList(stashEntity, stashList);

        for itemData in stashList {
            NGPlusItemStatDebug
                .DumpItemDataTierStats(
                    sys,
                    statsSystem,
                    itemData,
                    TDBID.ToStringDEBUG(ItemID.GetTDBID(itemData.GetID())),
                    "console-stash"
                );
        }
    }
}
