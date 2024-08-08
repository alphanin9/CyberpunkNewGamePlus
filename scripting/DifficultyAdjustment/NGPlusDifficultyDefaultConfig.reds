module NGPlus.DifficultyConfig

@if(ModuleExists("NGPlus.CustomConfig"))
import NGPlus.CustomConfig.*

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetFastUpgradeChance() -> Float = UserConfig.GetFastUpgradeChance();

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetTankUpgradeChance() -> Float = UserConfig.GetTankUpgradeChance();

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetRegenUpgradeChance() -> Float = UserConfig.GetRegenUpgradeChance();

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetOpticalCamoUpgradeChance() -> Float = UserConfig.GetOpticalCamoUpgradeChance();

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetNetrunnerUpgradeChance() -> Float = UserConfig.GetNetrunnerUpgradeChance();

// NOTE: Used to determine if you should start in the landfill or in V's room in Post-Heist start
@if(ModuleExists("NGPlus.CustomConfig"))
public func GetShouldFastForwardQ101Start() -> Bool = UserConfig.GetShouldFastForwardQ101Start();

// NOTE: currently unused, will be used in the future to upgrade enemy dodge abilities
@if(ModuleExists("NGPlus.CustomConfig"))
public func GetDodgeUpgradeChance() -> Float = UserConfig.GetDodgeUpgradeChance();

// NOTE: used to enable/disable random boss encounters people were unhappy with...
@if(ModuleExists("NGPlus.CustomConfig"))
public func GetShouldEnableRandomEncounters() -> Bool = UserConfig.GetShouldEnableRandomEncounters();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetFastUpgradeChance() -> Float = DefaultDifficultyConfig.GetFastUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetTankUpgradeChance() -> Float = DefaultDifficultyConfig.GetTankUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetRegenUpgradeChance() -> Float = DefaultDifficultyConfig.GetRegenUpgradeChance();

// Currently set to 0.0 by default...
@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetOpticalCamoUpgradeChance() -> Float = DefaultDifficultyConfig.GetOpticalCamoUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetNetrunnerUpgradeChance() -> Float = DefaultDifficultyConfig.GetNetrunnerUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetDodgeUpgradeChance() -> Float = DefaultDifficultyConfig.GetDodgeUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetShouldFastForwardQ101Start() -> Bool = DefaultDifficultyConfig.GetShouldFastForwardQ101Start();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetShouldEnableRandomEncounters() -> Bool = DefaultDifficultyConfig.GetShouldEnableRandomEncounters();

// Inherit your UserConfig from this to get default values
public abstract class DefaultDifficultyConfig {
    public static func GetFastUpgradeChance() -> Float = 77.0;
    public static func GetTankUpgradeChance() -> Float = 55.0;
    public static func GetRegenUpgradeChance() -> Float = 44.0;
    public static func GetOpticalCamoUpgradeChance() -> Float = 0.0;
    public static func GetNetrunnerUpgradeChance() -> Float = 50.0;
    public static func GetDodgeUpgradeChance() -> Float = 33.0;
    public static func GetShouldFastForwardQ101Start() -> Bool = false;
    public static func GetShouldEnableRandomEncounters() -> Bool = true;
}