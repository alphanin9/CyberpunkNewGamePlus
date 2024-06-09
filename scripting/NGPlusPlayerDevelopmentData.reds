module NGPlus.PlayerDevelopmentData

@addField(PlayerDevelopmentData)
let m_isInNgPlus: Bool;

@replaceMethod(PlayerDevelopmentData)
private const final func ModifyProficiencyLevel(proficiencyIndex: Int32, isDebug: Bool, opt levelIncrease: Int32) -> Void {
    let Blackboard: ref<IBlackboard>;
    let effectTags: array<CName>;
    let effects: array<ref<StatusEffect>>;
    let i: Int32;
    let level: LevelUpData;
    let statusEffectSys: ref<StatusEffectSystem>;
    if levelIncrease == 0 {
        levelIncrease = 1;
    }
    this.m_proficiencies[proficiencyIndex].currentLevel += levelIncrease;
    this.m_proficiencies[proficiencyIndex].currentExp = 0;
    this.m_proficiencies[proficiencyIndex].expToLevel = this.GetRemainingExpForLevelUp(this.m_proficiencies[proficiencyIndex].type);
    if !isDebug {
        this
            .ModifyDevPoints(
                this.m_proficiencies[proficiencyIndex].type,
                this.m_proficiencies[proficiencyIndex].currentLevel
            );
    }
    level.lvl = this.m_proficiencies[proficiencyIndex].currentLevel;
    level.type = this.m_proficiencies[proficiencyIndex].type;
    level.perkPoints = this.GetDevPoints(gamedataDevelopmentPointType.Primary);
    level.attributePoints = this.GetDevPoints(gamedataDevelopmentPointType.Attribute);
    level.espionagePoints = this.GetDevPoints(gamedataDevelopmentPointType.Espionage);
    this
        .SetProficiencyStat(
            this.m_proficiencies[proficiencyIndex].type,
            this.m_proficiencies[proficiencyIndex].currentLevel
        );
    this.ProcessProficiencyPassiveBonus(proficiencyIndex);
    if !this.m_isInNgPlus {
        Blackboard = GameInstance
            .GetBlackboardSystem(this.m_owner.GetGame())
            .Get(GetAllBlackboardDefs().UI_LevelUp);
        if IsDefined(Blackboard)
            && this.m_owner
                == GameInstance
                    .GetPlayerSystem(this.m_owner.GetGame())
                    .GetLocalPlayerMainGameObject() {
            Blackboard
                .SetVariant(GetAllBlackboardDefs().UI_LevelUp.level, ToVariant(level));
            Blackboard.SignalVariant(GetAllBlackboardDefs().UI_LevelUp.level);
        }
    }
    this.SetAchievementProgress(proficiencyIndex);
    if this.m_proficiencies[proficiencyIndex].currentLevel
        == RPGManager
            .GetProficiencyRecord(this.m_proficiencies[proficiencyIndex].type)
            .MaxLevel() {
        if Equals(
            this.m_proficiencies[proficiencyIndex].type,
            gamedataProficiencyType.StreetCred
        ) {
            this.SendMaxStreetCredLevelReachedTrackingRequest();
        } else {
            if NotEquals(
                this.m_proficiencies[proficiencyIndex].type,
                gamedataProficiencyType.Level
            )
                && NotEquals(
                    this.m_proficiencies[proficiencyIndex].type,
                    gamedataProficiencyType.Espionage
                ) {
                this.CheckSpecialistAchievement(proficiencyIndex);
            }
        }
    }
    if Equals(this.m_proficiencies[proficiencyIndex].type, gamedataProficiencyType.Level) {
        this.ProcessTutorialFacts();
        if Equals(
            GameInstance.GetStatsDataSystem(this.m_owner.GetGame()).GetDifficulty(),
            gameDifficulty.Story
        ) {
            GameInstance
                .GetStatPoolsSystem(this.m_owner.GetGame())
                .RequestSettingStatPoolValue(
                    Cast<StatsObjectID>(this.m_owner.GetEntityID()),
                    gamedataStatPoolType.Health,
                    100.0,
                    this.m_owner
                );
            statusEffectSys = GameInstance.GetStatusEffectSystem(this.m_owner.GetGame());
            statusEffectSys.GetAppliedEffects(this.m_owner.GetEntityID(), effects);
            i = 0;

            while i < ArraySize(effects) {
                effectTags = effects[i].GetRecord().GameplayTags();
                if effects[i].GetRemainingDuration() > 0.0 && ArrayContains(effectTags, n"Debuff") {
                    statusEffectSys
                        .RemoveStatusEffect(
                            this.m_owner.GetEntityID(),
                            effects[i].GetRecord().GetID(),
                            effects[i].GetStackCount()
                        );
                }
                i += 1;
            }
        }
    }
}

@replaceMethod(PlayerDevelopmentData)
public const final func AddExperience(
    amount: Int32,
    type: gamedataProficiencyType,
    telemetryGainReason: telemetryLevelGainReason,
    opt isDebug: Bool
) -> Void {
    let awardedAmount: Int32;
    let proficiencyProgress: ref<ProficiencyProgressEvent>;
    let reqExp: Int32;
    let telemetryEvt: TelemetryLevelGained;
    let pIndex: Int32 = this.GetProficiencyIndexByType(type);
    if pIndex >= 0 && !this.IsProficiencyMaxLvl(type) {
        while amount > 0 && !this.IsProficiencyMaxLvl(type) {
            reqExp = this.GetRemainingExpForLevelUp(type);
            if amount - reqExp >= 0 {
                awardedAmount += reqExp;
                amount -= reqExp;
                this.m_proficiencies[pIndex].currentExp += reqExp;
                this.m_proficiencies[pIndex].expToLevel = this.GetRemainingExpForLevelUp(type);
                if this.CanGainNextProficiencyLevel(pIndex) {
                    this.ModifyProficiencyLevel(type, isDebug);
                    this.UpdateUIBB();
                    if this.m_owner.IsPlayerControlled()
                        && NotEquals(telemetryGainReason, telemetryLevelGainReason.Ignore) {
                        telemetryEvt.playerPuppet = this.m_owner;
                        telemetryEvt.proficiencyType = type;
                        telemetryEvt.proficiencyValue = this.m_proficiencies[pIndex].currentLevel;
                        telemetryEvt.isDebugEvt = Equals(telemetryGainReason, telemetryLevelGainReason.IsDebug);
                        telemetryEvt.perkPointsAwarded = this
                            .GetDevPointsForLevel(
                                this.m_proficiencies[pIndex].currentLevel,
                                type,
                                gamedataDevelopmentPointType.Primary
                            );
                        telemetryEvt.attributePointsAwarded = this
                            .GetDevPointsForLevel(
                                this.m_proficiencies[pIndex].currentLevel,
                                type,
                                gamedataDevelopmentPointType.Attribute
                            );
                        GameInstance
                            .GetTelemetrySystem(this.m_owner.GetGame())
                            .LogLevelGained(telemetryEvt);
                    }
                } else {
                    return;
                }
            } else {
                this.m_proficiencies[pIndex].currentExp += amount;
                this.m_proficiencies[pIndex].expToLevel = this.GetRemainingExpForLevelUp(type);
                awardedAmount += amount;
                amount -= amount;
            }
        }
        if awardedAmount > 0 {
            if this.m_displayActivityLog {
                if Equals(type, gamedataProficiencyType.StreetCred)
                    && GameInstance
                        .GetQuestsSystem(this.m_owner.GetGame())
                        .GetFact(n"street_cred_tutorial") == 0
                    && GameInstance
                        .GetQuestsSystem(this.m_owner.GetGame())
                        .GetFact(n"disable_tutorials") == 0
                    && Equals(telemetryGainReason, telemetryLevelGainReason.Gameplay)
                    && GameInstance
                        .GetQuestsSystem(this.m_owner.GetGame())
                        .GetFact(n"q001_show_sts_tut") > 0 {
                    GameInstance
                        .GetQuestsSystem(this.m_owner.GetGame())
                        .SetFact(n"street_cred_tutorial", 1);
                }
            }
            
            if !this.m_isInNgPlus {
                // I fucking loathe those notifications
                proficiencyProgress = new ProficiencyProgressEvent();
                proficiencyProgress.type = type;
                proficiencyProgress.expValue = this.GetCurrentLevelProficiencyExp(type);
                proficiencyProgress.delta = awardedAmount;
                proficiencyProgress.remainingXP = this.GetRemainingExpForLevelUp(type);
                proficiencyProgress.currentLevel = this.GetProficiencyLevel(type);
                proficiencyProgress.isLevelMaxed = this.GetProficiencyLevel(type) + 1 == this.GetProficiencyAbsoluteMaxLevel(type);

                GameInstance
                .GetUISystem(this.m_owner.GetGame())
                .QueueEvent(proficiencyProgress);
            }
            
            if Equals(type, gamedataProficiencyType.Level) {
                this.UpdatePlayerXP();
            }
        }
    }
}

@replaceMethod(PlayerDevelopmentData)
public final func RefreshDevelopmentSystem() {
    this.RefreshProficiencyStats();
    this.SetAttributes();
    this.UpdateProficiencyMaxLevels();

    // By the time this gets called, quests system is already executing...
    let isNgPlus = GameInstance.GetQuestsSystem(GetGameInstance()).GetFactStr("ngplus_active") == 1;

    // Fix for Corpo progression build adding extra money :P
    // Could be done simpler, I think
    if isNgPlus {
        this.SetProgressionBuild(gamedataBuildType.StartingBuild);
        return;
    }

    if Equals(this.GetLifePath(), gamedataLifePath.StreetKid) {
      this.SetProgressionBuild(gamedataBuildType.StreetKidStarting);
    } else {
      if Equals(this.GetLifePath(), gamedataLifePath.Nomad) {
        this.SetProgressionBuild(gamedataBuildType.NomadStarting);
      } else {
        if Equals(this.GetLifePath(), gamedataLifePath.Corporate) {
          this.SetProgressionBuild(gamedataBuildType.CorporateStarting);
        } else {
          this.SetProgressionBuild(gamedataBuildType.StartingBuild);
        };
      };
    };
}