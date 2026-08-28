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
}

// Console command. Run after the transfer has fully settled - the apply-time dump happens
// before the game's own retrofix passes get a look at the items, so this is the one that says
// where they actually ended up.
public static exec func NGPlusDumpItemTiers(gameInstance: GameInstance) {
    let itemList: array<wref<gameItemData>>;
    let sys = GameInstance.GetNewGamePlusSystem();
    let statsSystem = GameInstance.GetStatsSystem(gameInstance);

    GameInstance.GetTransactionSystem(gameInstance).GetItemList(GetPlayer(gameInstance), itemList);

    for itemData in itemList {
        NGPlusItemStatDebug
            .DumpLiveTierStats(
                sys,
                statsSystem,
                itemData.GetStatsObjectID(),
                TDBID.ToStringDEBUG(ItemID.GetTDBID(itemData.GetID())),
                "console"
            );
    }
}
