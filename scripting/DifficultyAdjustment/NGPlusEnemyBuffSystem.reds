module NGPlus.Difficulty.System

class PuppetPowerUpSystem {
    private let m_puppet: ref<NPCPuppet>;
    private let m_statsSystem: ref<StatsSystem>;
    private let m_targetPowerLevel: Float;

    private final func RaisePowerLevel() {
        let puppetStatId = Cast<StatsObjectID>(this.m_puppet.GetEntityID());
        let currentPowerLevel = this.m_statsSystem.GetStatValue(puppetStatId, gamedataStatType.PowerLevel);

        if currentPowerLevel >= this.m_targetPowerLevel {
            return;
        }

        let neededModifierValue = this.m_targetPowerLevel - currentPowerLevel;
        let statModifier = RPGManager.CreateStatModifier(gamedataStatType.PowerLevel, gameStatModifierType.Additive, neededModifierValue);

        this.m_statsSystem.AddModifier(puppetStatId, statModifier);
    }

    private final func ApplyAbilityGroup(aGroupId: TweakDBID) {
        let abilityGroup = TweakDBInterface.GetGameplayAbilityGroupRecord(aGroupId);

        if !IsDefined(abilityGroup) {
            ModLog(n"NG+", "Ability group is missing!");
            return;
        }

        RPGManager.ApplyAbilityGroup(this.m_puppet, abilityGroup);
    }

    private final func RandMeetsChance(aChance: Float) -> Bool {
        return RandRangeF(0.0, 1.0) < aChance;
    }

    private final func AddCyberAbilities() {
        // Only buff the normies, don't buff bosses because they're already pretty strong and drones because I don't care about them
        if this.m_puppet.IsBoss() || this.m_puppet.IsMaxTac() || this.m_puppet.IsDrone() {
            return;
        }

        if this.m_puppet.IsNetrunnerPuppet() && this.RandMeetsChance(0.5) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Netrunner");
        }

        if this.RandMeetsChance(0.77) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Fast");
        }

        if this.RandMeetsChance(0.55) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Tanky");
        }

        if this.RandMeetsChance(0.44) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Regen");
        }

        if this.RandMeetsChance(0.25) {
            this.ApplyAbilityGroup(t"ArchetypeData.NGPlusAbilityGroup_Sneaky");
        }
    }

    public final func PowerUp() {
        this.RaisePowerLevel();
        this.AddCyberAbilities();
    }

    public final func SetTargetPowerLevel(aLevel: Float) {
        this.m_targetPowerLevel = aLevel;
    }

    public final func SetPuppet(aPuppet: ref<NPCPuppet>) {
        this.m_puppet = aPuppet;
        this.m_statsSystem = GameInstance.GetStatsSystem(this.m_puppet.GetGame());
    }

    public static func Create(aPuppet: ref<NPCPuppet>, aLevel: Float) -> ref<PuppetPowerUpSystem> {
        let ret = new PuppetPowerUpSystem();

        ret.SetPuppet(aPuppet);
        ret.SetTargetPowerLevel(aLevel);

        return ret;
    }
}

class NGPlusDifficultySystem extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;
    private let m_statsSystem: ref<StatsSystem>;
    private let m_player: ref<PlayerPuppet>;

    private let m_hasDetectedNgPlus: Bool;
    private let m_ngPlusActive: Bool;
    private let m_playerMaxLevel: Float;
    private let m_isEP1: Bool;

    private func OnAttach() {
        this.m_questsSystem = GameInstance.GetQuestsSystem(this.GetGameInstance());
        this.m_statsSystem = GameInstance.GetStatsSystem(this.GetGameInstance());
        
        // Kinda broken with non-EP1, but w/e...
        this.m_playerMaxLevel = Cast<Float>(TweakDBInterface.GetInt(t"Proficiencies.Level.maxLevel", 50));
        
        GameInstance.GetCallbackSystem().RegisterCallback(n"Entity/Attached", this, n"OnNPCSpawned")
            .AddTarget(EntityTarget.Type(n"NPCPuppet"));
    }

    private cb func OnNPCSpawned(event: ref<EntityLifecycleEvent>) {
        if !this.m_hasDetectedNgPlus {
            this.m_player = GetPlayer(this.GetGameInstance());
            this.m_ngPlusActive = this.m_questsSystem.GetFactStr("ngplus_active") == 1;
            this.m_hasDetectedNgPlus = true;
        }

        if !this.m_ngPlusActive {
            return;
        }

        // Not for VR Tutorial (though we'll be disabling it...), Johnny, Kurtz or Aguilar
        if this.m_player.IsReplacer() {
            return;
        }

        let puppet = event.GetEntity() as NPCPuppet;

        if !IsDefined(puppet) {
            return;
        }

        if !puppet.IsAggressive() {
            return;
        }

        let powerUpSystem = PuppetPowerUpSystem.Create(puppet, this.m_playerMaxLevel);

        powerUpSystem.PowerUp();
    }
}