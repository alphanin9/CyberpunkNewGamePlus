module NGPlus.Difficulty.System

import NGPlus.DifficultyConfig.*
import NGPlus.*

class PuppetPowerUpSystem {
    private let m_puppet: ref<NPCPuppet>;
    private let m_statsSystem: ref<StatsSystem>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_targetPowerLevel: Float;

    private final func RaisePowerLevel() {
        // Disabled for now, causes various issues...
        // We can increase general NPC toughness in other ways

        /*let puppetStatId = Cast<StatsObjectID>(this.m_puppet.GetEntityID());
        let currentPowerLevel = this.m_statsSystem.GetStatValue(puppetStatId, gamedataStatType.PowerLevel);

        if currentPowerLevel >= this.m_targetPowerLevel {
            return;
        }

        let neededModifierValue = this.m_targetPowerLevel - currentPowerLevel;
        let statModifier = RPGManager.CreateStatModifier(gamedataStatType.PowerLevel, gameStatModifierType.Additive, neededModifierValue);

        this.m_statsSystem.AddModifier(puppetStatId, statModifier);*/
    }

    private final func ApplyAbilityGroup(aGroupId: TweakDBID) {
        // FIX: multiple armor levels stacking together?
        let hasSubdermalArmor = this.m_statsSystem.GetStatValue(Cast<StatsObjectID>(this.m_puppet.GetEntityID()), gamedataStatType.HasSubdermalArmor) > 0.0;

        let abilityGroup = TweakDBInterface.GetGameplayAbilityGroupRecord(aGroupId);

        if !IsDefined(abilityGroup) {
            this.m_ngPlusSystem.Spew("Ability group is missing!");
            return;
        }

        let subdermalArmorAbilities = [t"Ability.HasSubdermalArmorBase", t"Ability.HasSubdermalArmorMedium", t"Ability.HasSubdermalArmorHigh"];   

        let outList: [wref<GameplayAbility_Record>];
        abilityGroup.Abilities(outList);

        for ability in outList {
            let isArmorAbility = ArrayContains(subdermalArmorAbilities, ability.GetID());

            if !isArmorAbility || !hasSubdermalArmor {
                RPGManager.ApplyAbility(this.m_puppet, ability);
                if isArmorAbility {
                    hasSubdermalArmor = true;
                }
            }
        }
    }

    private final func RandMeetsChance(aChance: Float) -> Bool {
        return RandRangeF(0.0, 1.0) < aChance;
    }

    // Smasher actually has psycho tag, WTF???
    // Explains NG+ Smasher being a bitch
    private final func IsSmasher() -> Bool {
        return Equals(this.m_puppet.GetRecord().ArchetypeName(), n"adamsmasher");
    }

    // Yeah, I kinda fucked up
    private final func IsNGPlusBoss() -> Bool {
        return NPCManager.HasTag(this.m_puppet.GetRecordID(), n"NGPlusBoss");
    }

    private final func AddCyberAbilities() {
        let isPsycho = this.m_puppet.IsCharacterCyberpsycho() && !this.IsSmasher() && !this.IsNGPlusBoss();

        // Only buff the normies, don't buff bosses because they're already pretty strong (with the exception of psychos) and drones because I don't care about them
        if (this.m_puppet.IsBoss() && !isPsycho) || this.m_puppet.IsMaxTac() || this.m_puppet.IsDrone() {
            return;
        }

        let settings = UserSettings.Get();

        let netrunnerChance = settings.netrunnerUpgradeChance / 100.0;
        let fastChance = settings.fastUpgradeChance / 100.0;
        let tankChance = settings.tankUpgradeChance / 100.0;
        let regenChance = settings.regenUpgradeChance / 100.0;
        let stealthChance = settings.opticalCamoUpgradeChance / 100.0;
        let dodgeChance = settings.dodgeUpgradeChance / 100.0;

        if netrunnerChance > 0.0 && this.RandMeetsChance(netrunnerChance) && this.m_puppet.IsNetrunnerPuppet() {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Netrunner");
        }

        if fastChance > 0.0 && this.RandMeetsChance(fastChance) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Fast");
        }

        if tankChance > 0.0 && this.RandMeetsChance(tankChance) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Tanky");
        }

        // Psychos are very busted with regen, as their health pool is fuckhuge
        if regenChance > 0.0 && this.RandMeetsChance(regenChance) && !isPsycho {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Regen");
        }

        // FIX: unexpected units getting Optical Camo
        if stealthChance > 0.0 && this.RandMeetsChance(stealthChance) && !this.m_puppet.IsMech() {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Sneaky");
        }

        if dodgeChance > 0.0 && this.RandMeetsChance(dodgeChance) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Dodger");
        }
    }

    public final func PowerUp() {
        this.RaisePowerLevel();
        this.AddCyberAbilities();
    }

    public final func SetTargetPowerLevel(aLevel: Float) {
        this.m_targetPowerLevel = aLevel;
    }

    public final func SetPuppetAndInitializeSystems(aPuppet: ref<NPCPuppet>) {
        this.m_puppet = aPuppet;
        this.m_statsSystem = GameInstance.GetStatsSystem(this.m_puppet.GetGame());
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
    }

    public static func Create(aPuppet: ref<NPCPuppet>, aLevel: Float) -> ref<PuppetPowerUpSystem> {
        let ret = new PuppetPowerUpSystem();

        ret.SetPuppetAndInitializeSystems(aPuppet);
        ret.SetTargetPowerLevel(aLevel);

        return ret;
    }
}

class NGPlusDifficultySystem extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;
    private let m_statsSystem: ref<StatsSystem>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_player: ref<PlayerPuppet>;

    private let m_playerMaxLevel: Float;

    private func OnAttach() {
        this.m_questsSystem = GameInstance.GetQuestsSystem(this.GetGameInstance());
        this.m_statsSystem = GameInstance.GetStatsSystem(this.GetGameInstance());
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        
        // Kinda broken with non-EP1, but w/e...
        this.m_playerMaxLevel = Cast<Float>(TweakDBInterface.GetInt(t"Proficiencies.Level.maxLevel", 50));
        
        GameInstance.GetCallbackSystem().RegisterCallback(n"Entity/Attached", this, n"OnNPCSpawned")
            .AddTarget(EntityTarget.Type(n"NPCPuppet"));
    }

    private cb func OnNPCSpawned(event: ref<EntityLifecycleEvent>) {
        let puppet = event.GetEntity() as NPCPuppet;

        if !IsDefined(puppet) {
            return;
        }

        if this.m_questsSystem.GetFactStr("ngplus_active") == 0 {
            return;
        }

        this.m_player = GetPlayer(this.GetGameInstance()); // FIX: breaking with situations where system doesn't attach between V/replacer swaps

        // Not for VR Tutorial (though we'll be disabling it...), Johnny, Kurtz or Aguilar
        if this.m_player.IsReplacer() {
            return;
        }

        if !puppet.IsAggressive() {
            return;
        }

        let powerUpSystem = PuppetPowerUpSystem.Create(puppet, this.m_playerMaxLevel);

        powerUpSystem.PowerUp();
    }
}