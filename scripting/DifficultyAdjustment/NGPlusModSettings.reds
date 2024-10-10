module NGPlus

public enum ENGPlusQ101StartPoint {
    Default = 0,
    AtVApartment = 1
}

public enum ENGPlusDynamicSpawnSystemLimit {
    Default = 0,
    Low = 1,
    Medium = 2,
    High = 3
}

public class UserSettings {
    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_Q101StartPoint_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_Q101StartPoint_Desc")
    @runtimeProperty("ModSettings.displayValues.Default", "NewGamePlus_Q101StartPoint_Default")
    @runtimeProperty("ModSettings.displayValues.AtVApartment", "NewGamePlus_Q101StartPoint_AtVApartment")
    public let q101StartPoint: ENGPlusQ101StartPoint = ENGPlusQ101StartPoint.Default;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_FastUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_FastUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let fastUpgradeChance: Float = 77.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_TankUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_TankUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let tankUpgradeChance: Float = 55.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_RegenUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_RegenUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let regenUpgradeChance: Float = 44.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_NetrunnerUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_NetrunnerUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let netrunnerUpgradeChance: Float = 50.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_DodgeUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_DodgeUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let dodgeUpgradeChance: Float = 33.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_Difficulty_Name")
    @runtimeProperty("ModSettings.category.order", "1")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_CamoUpgradeChance_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_CamoUpgradeChance_Desc")
    @runtimeProperty("ModSettings.step", "0.5")
    @runtimeProperty("ModSettings.min", "0.0")
    @runtimeProperty("ModSettings.max", "100.0")
    public let opticalCamoUpgradeChance: Float = 0.0;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_RandomEncounters_Name")
    @runtimeProperty("ModSettings.category.order", "2")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_EnableRandomEncounters_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_EnableRandomEncounters_Desc")
    public let enableRandomEncounters: Bool = true;

    @runtimeProperty("ModSettings.mod", "NewGamePlus_MainMenuButton")
    @runtimeProperty("ModSettings.category", "NewGamePlus_RandomEncounters_Name")
    @runtimeProperty("ModSettings.category.order", "2")
    @runtimeProperty("ModSettings.displayName", "NewGamePlus_DynamicSpawnSystemLimit_Name")
    @runtimeProperty("ModSettings.description", "NewGamePlus_DynamicSpawnSystemLimit_Desc")
    @runtimeProperty("ModSettings.displayValues.Default", "NewGamePlus_DynamicSpawnSystemLimit_Default")
    @runtimeProperty("ModSettings.displayValues.Low", "NewGamePlus_DynamicSpawnSystemLimit_Low")
    @runtimeProperty("ModSettings.displayValues.Medium", "NewGamePlus_DynamicSpawnSystemLimit_Medium")
    @runtimeProperty("ModSettings.displayValues.High", "NewGamePlus_DynamicSpawnSystemLimit_High")
    public let dynamicSpawnSystemLimit: ENGPlusDynamicSpawnSystemLimit = ENGPlusDynamicSpawnSystemLimit.High;

    public static final func Get() -> ref<UserSettings> {
        return new UserSettings();
    }
}

public class DynamicSpawnSystemCustomizer extends ScriptableTweak {
    protected cb func OnApply() {
        let settings = UserSettings.Get();

        let dynamicSpawnSystemCount = 20;

        switch(settings.dynamicSpawnSystemLimit) {
            case ENGPlusDynamicSpawnSystemLimit.Default:
                break;
            case ENGPlusDynamicSpawnSystemLimit.Low:
                dynamicSpawnSystemCount = 40;
                break;
            case ENGPlusDynamicSpawnSystemLimit.Medium:
                dynamicSpawnSystemCount = 60;
                break;
            case ENGPlusDynamicSpawnSystemLimit.High:
                dynamicSpawnSystemCount = 100;
                break;
        }

        GameInstance.GetNewGamePlusSystem().Spew(s"Applied dynamic spawn system amt: \(dynamicSpawnSystemCount)");
        
        TweakDBManager.SetFlat(t"DynamicSpawnSystem.setup.totalEntitiesLimit", dynamicSpawnSystemCount);
        TweakDBManager.SetFlat(t"DynamicSpawnSystem.setup.numberOfDeadBodiesToTriggerImmediateDespawn", dynamicSpawnSystemCount);
    }
}