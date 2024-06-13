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

@if(ModuleExists("NGPlus.CustomConfig"))
public func GetDodgeUpgradeChance() -> Float = UserConfig.GetDodgeUpgradeChance();

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetFastUpgradeChance() -> Float = 77.0;

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetTankUpgradeChance() -> Float = 55.0;

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetRegenUpgradeChance() -> Float = 44.0;

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetOpticalCamoUpgradeChance() -> Float = 20.0;

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetNetrunnerUpgradeChance() -> Float = 50.0;

@if(!ModuleExists("NGPlus.CustomConfig"))
public func GetDodgeUpgradeChance() -> Float = 33.0;

// Inherit your UserConfig from this to get default values!
public abstract class DefaultDifficultyConfig {
    public static func GetFastUpgradeChance() -> Float = GetFastUpgradeChance();
    public static func GetTankUpgradeChance() -> Float = GetTankUpgradeChance();
    public static func GetRegenUpgradeChance() -> Float = GetRegenUpgradeChance();
    public static func GetOpticalCamoUpgradeChance() -> Float = GetOpticalCamoUpgradeChance();
    public static func GetNetrunnerUpgradeChance() -> Float = GetNetrunnerUpgradeChance();
    public static func GetDodgeUpgradeChance() -> Float = GetDodgeUpgradeChance();
}