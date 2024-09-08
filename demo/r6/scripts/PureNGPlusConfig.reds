module NGPlus.CustomConfig

import NGPlus.DifficultyConfig.*

// Mostly purist config, only non-base thing is Q101 start sending you to Misty wheeling you in...
public class UserConfig extends DefaultDifficultyConfig {
    public static func GetFastUpgradeChance() -> Float = 0.0;
    public static func GetTankUpgradeChance() -> Float = 0.0;
    public static func GetRegenUpgradeChance() -> Float = 0.0;
    public static func GetOpticalCamoUpgradeChance() -> Float = 0.0;
    public static func GetNetrunnerUpgradeChance() -> Float = 0.0;
    public static func GetDodgeUpgradeChance() -> Float = 0.0;
    public static func GetShouldFastForwardQ101Start() -> Bool = true;
    public static func GetShouldEnableRandomEncounters() -> Bool = false;
}