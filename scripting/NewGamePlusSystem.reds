// NOTE: derive these from ISerializable/IScriptable so we can pass them by ref/wref instead of copying them around everywhere
public native struct RedItemData {
    native let itemId: ItemID;
    native let itemQuantity: Int32;
    native let attachments: [ItemID];
    native let statModifiers: [ref<gameStatModifierData>];
}

public native struct RedCraftInfo {
    native let targetItem: TweakDBID;
    native let amount: Int32;
    native let hideOnItemsAdded: [ItemID];
}

public native struct WardrobeEntry {
    native let appearanceName: CName;
    native let itemId: ItemID;
}

public native struct PlayerSaveData {
    native let isValid: Bool;
    native let playerPerkPoints: Int32;
    native let playerRelicPoints: Int32;
    native let playerAttributePoints: Int32;
    native let playerBodyAttribute: Int32;
    native let playerReflexAttribute: Int32;
    native let playerTechAttribute: Int32;
    native let playerIntelligenceAttribute: Int32;
    native let playerCoolAttribute: Int32;
    native let playerBodySkillLevel: Int32;
    native let playerReflexSkillLevel: Int32;
    native let playerTechSkillLevel: Int32;
    native let playerIntelligenceSkillLevel: Int32;
    native let playerCoolSkillLevel: Int32;
    native let playerLevel: Int32;
    native let playerStreetCred: Int32;
    native let playerMoney: Int32;
    native let playerItems: [RedItemData];
    native let playerStashItems: [RedItemData];
    native let playerEquippedOperatingSystem: ItemID;
    native let playerEquippedKiroshis: ItemID;
    native let playerEquippedLegCyberware: ItemID;
    native let playerEquippedArmCyberware: ItemID;
    native let playerEquippedCardiacSystemCW: [ItemID];
    native let playerVehicleGarage: [TweakDBID];

    native let knownRecipeTargetItems: [RedCraftInfo];

    // Done this way to make sure the player's CW cap modifiers look the same as basegame
    native let playerCyberwareCapacity: [Float];

    // Currently unused
    native let playerCarryCapacity: [Float];

    native let wardrobeEntries: [WardrobeEntry];
}

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

    // Maybe I should pass it around by wref?
    public native func GetSaveData() -> PlayerSaveData;

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