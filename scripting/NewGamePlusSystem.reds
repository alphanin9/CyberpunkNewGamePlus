// NOTE: derive these from ISerializable/IScriptable so we can pass them by ref/wref instead of copying them around everywhere

// Begin crafting
public native class NGPlusCraftingInfo extends IScriptable {
    public native func GetTargetItem() -> TweakDBID;
    public native func GetAmount() -> Int32;
    public native func GetHideOnItemsAdded() -> [ItemID];
}

public native class CraftingSystemResults extends IScriptable {
    public native func GetData() -> [ref<NGPlusCraftingInfo>];
}
// End crafting

// Begin EqEx outfit system
public native class NGPlusOutfitPart extends IScriptable {
    public native func GetSlotID() -> TweakDBID;
    public native func GetItemID() -> ItemID;
}

public native class NGPlusOutfitSet extends IScriptable {
    public native func GetName() -> CName;
    public native func GetOutfitParts() -> [ref<NGPlusOutfitPart>];
}

public native class OutfitSystemResults extends IScriptable {
    public native func GetData() -> [ref<NGPlusOutfitSet>];
}
// End EqEx outfit system

// Begin EqEx view state system
public native class ViewManagerResults extends IScriptable {
    public native func GetSource() -> Uint64;
}
// End EqEx view state system

// Begin equipment system
public native class EquipmentSystemResults extends IScriptable {
    public native func GetPlayerEquippedOperatingSystem() -> ItemID;
    public native func GetPlayerEquippedKiroshis() -> ItemID;
    public native func GetPlayerEquippedLegCyberware() -> ItemID;
    public native func GetPlayerEquippedArmCyberware() -> ItemID;
    public native func GetPlayerEquippedCardiacSystemCW() -> [ItemID];
}
// End equipment system

// Begin inventory
public native class NGPlusItemData extends IScriptable {
    public native func GetItemId() -> ItemID;
    public native func GetItemQuantity() -> Int32;
    public native func GetAttachments() -> [ItemID];
    public native func GetStatModifiers() -> [ref<gameStatModifierData>];
}

public native class InventoryReaderResults extends IScriptable {
    public native func GetInventory() -> [ref<NGPlusItemData>];
    public native func GetStash() -> [ref<NGPlusItemData>];
    public native func GetMoney() -> Int32;
}
// End inventory

// Begin player development system
public native class PlayerDevelopmentSystemResults extends IScriptable {
    public native func GetPerkPoints() -> Int32;
    public native func GetRelicPoints() -> Int32;
    public native func GetAttributePoints() -> Int32;
}
// End player development system

// Begin stats system
public native class StatsSystemResults extends IScriptable {
    public native func GetLevel() -> Float;
    public native func GetStreetCred() -> Float;
    
    public native func GetReflexes() -> Float;
    public native func GetBody() -> Float;
    public native func GetTechnicalAbility() -> Float;
    public native func GetIntelligence() -> Float;
    public native func GetCool() -> Float;

    public native func GetReflexesSkill() -> Float;
    public native func GetBodySkill() -> Float;
    public native func GetTechnicalAbilitySkill() -> Float;
    public native func GetIntelligenceSkill() -> Float;
    public native func GetCoolSkill() -> Float;

    public native func GetCyberwareCapacity() -> [Float];
}
// End stats system

// Begin vehicle garage
public native class VehicleGarageResults extends IScriptable {
    public native func GetGarage() -> [TweakDBID];
}
// End vehicle garage

// Begin wardrobe
public native class WardrobeResults extends IScriptable {
    public native func GetWardrobe() -> [ItemID];
}
// End wardrobe

// Begin aggregator
public native class NGPlusProgressionData extends IScriptable {
    public native func GetCraftingSystemResults() -> ref<CraftingSystemResults>;
    public native func GetEquipmentExOutfitSystemResults() -> ref<OutfitSystemResults>;
    public native func GetEquipmentExViewManagerResults() -> ref<ViewManagerResults>;
    public native func GetEquipmentSystemResults() -> ref<EquipmentSystemResults>;
    public native func GetPlayerInventory() -> ref<InventoryReaderResults>;
    public native func GetPlayerDevelopmentSystemResults() -> ref<PlayerDevelopmentSystemResults>;
    public native func GetStatsSystemResults() -> ref<StatsSystemResults>;
    public native func GetVehicleGarageResults() -> ref<VehicleGarageResults>;
    public native func GetWardrobeResults() -> ref<WardrobeResults>;
}
// End aggregator

enum ENewGamePlusStartType {
    StartFromQ001 = 0,
    StartFromQ101 = 1,
    StartFromQ001_NoEP1 = 2,
    StartFromQ101_NoEP1 = 3,
    StartFromQ101_ProgressionBuild = 4,
    StartFromQ101_ProgressionBuild_NoEP1 = 5,
    Count = 6,
    Invalid = 7
}

public native class NewGamePlusSystem extends IGameSystem {
    public native func HasPointOfNoReturnSave() -> Bool;

    public native func ParsePointOfNoReturnSaveData(saveName: script_ref<String>) -> Bool;

    public native func GetNewGamePlusState() -> Bool;

    public native func SetNewGamePlusState(newState: Bool) -> Void;

    public native func GetProgressionData() -> ref<NGPlusProgressionData>;

    public native func SetNewGamePlusGameDefinition(startType: ENewGamePlusStartType) -> Void ;

    public native func IsSaveValidForNewGamePlus(saveName: script_ref<String>) -> Bool;

    public native func ResolveNewGamePlusSaves(saves: script_ref<[String]>) -> [Int32];

    public native func LoadExpansionIntoSave() -> Void;
    // Since LogChannel is not declared for everybody...

    public native func Spew(str: script_ref<String>) -> Void;
    public native func Error(str: script_ref<String>) -> Void;

    public native func GetStandaloneState() -> Bool;
    public native func SetStandaloneState(aNewState: Bool) -> Void;

    public native func IsInNewGamePlusSave() -> Bool;
}

@addMethod(GameInstance)
public native static func GetNewGamePlusSystem() -> ref<NewGamePlusSystem>;