module NGPlus.CustomConfig

import NGPlus.DifficultyConfig.*

// Inherit UserConfig class from DefaultDifficultyConfig to get default values ETC...
public class UserConfig extends DefaultDifficultyConfig {
    // Upgrades can be stacked
    // All enemies can get upgrades, outside of non-cyberpsycho bosses and MaxTac (outside of health regeneration, as it really sucks on bosses)
    // You can disable an upgrade by setting its application chance to 0

    // Enemy upgrades should be reset on save reload

    // Returned value is how often the upgrade should be applied 
    public static func GetFastUpgradeChance() -> Float = 95.0; // (Sandy, dodge, dash, whatever else...)

    // How often the "tanky" upgrade should be applied (heavy armor and a tiny bit of regen)
    public static func GetTankUpgradeChance() -> Float = 60.0;

    // How often the "regen" upgrade should be applied (fast regen until enemy loses a bunch of health, at which point they won't regenerate anymore)
    public static func GetRegenUpgradeChance() -> Float = 25.0;

    // How often an enemy should get optical camo
    public static func GetOpticalCamoUpgradeChance() -> Float = 35.0;

    // How often a netrunner should be able to get the ability to do basically all quickhacks instead of their default set
    public static func GetNetrunnerUpgradeChance() -> Float = 100.0;

    // Currently unused, will be used in the future to upgrade enemy dodge abilities
    public static func GetDodgeUpgradeChance() -> Float = 0.0;

    // Not an upgrade, decides whether or not NG+ should start the Post-Heist start as normal (waking up in the trash) or with Misty wheeling you into V's apt
    // True = Misty wheeling you in
    // False = default behavior
    public static func GetShouldFastForwardQ101Start() -> Bool = true;

    // Maybe eventually random encounters can be enabled/disabled here, but I'm not sure if I want to do that    
}

