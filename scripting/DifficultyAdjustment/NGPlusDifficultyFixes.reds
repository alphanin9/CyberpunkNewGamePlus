module NGPlus.Difficulty.Correction

// Unused now that we don't play with power level!!!
// Not deleted to keep file structure same for manual installs (could set up migration in RED4ext plugin, but later?)

/*@replaceMethod(DamageSystem)
private final func ProcessStealthAttack(hitEvent: ref<gameHitEvent>) {
    let canStealthHit: Bool;
    let hitNotQuickMelee: Bool;
    let stealthHitDamageBonus: Float;
    let stealthHitMult: Float;
    let player: wref<PlayerPuppet> = hitEvent.attackData.GetInstigator() as PlayerPuppet;
    let isNgPlus = GameInstance.GetQuestsSystem(GetGameInstance()).GetFactStr("ngplus_active") == 1;
    if IsDefined(player) && IsDefined(hitEvent.target as ScriptedPuppet) {
        if IsDefined(hitEvent.attackData.GetWeapon()) {
            if !AttackData.IsPlayerInCombat(hitEvent.attackData)
                || StatusEffectHelper.HasStatusEffectWithTagConst(player, n"ExtendedStealth") {
                canStealthHit = GameInstance
                    .GetStatsSystem(GetGameInstance())
                    .GetStatValue(
                        Cast<StatsObjectID>(hitEvent.attackData.GetWeapon().GetEntityID()),
                        gamedataStatType.CanSilentKill
                    ) > 0.0;
                hitNotQuickMelee = NotEquals(hitEvent.attackData.GetAttackType(), gamedataAttackType.QuickMelee);
                if canStealthHit && hitNotQuickMelee {
                    let powerDifferential = RPGManager.CalculatePowerDifferential(hitEvent.target);
                    let isTough = Equals(powerDifferential, gameEPowerDifferential.IMPOSSIBLE);
                    if !isTough || isNgPlus {
                        stealthHitDamageBonus = GameInstance
                            .GetStatsSystem(GetGameInstance())
                            .GetStatValue(
                                Cast<StatsObjectID>(hitEvent.attackData.GetInstigator().GetEntityID()),
                                gamedataStatType.StealthHitDamageBonus
                            );
                        stealthHitMult = GameInstance
                            .GetStatsSystem(GetGameInstance())
                            .GetStatValue(
                                Cast<StatsObjectID>(hitEvent.attackData.GetInstigator().GetEntityID()),
                                gamedataStatType.StealthHitDamageMultiplier
                            );
                        if isTough {
                            stealthHitDamageBonus *= 0.5;
                        }
                        hitEvent
                            .attackComputed
                            .AddAttackValue(stealthHitDamageBonus, gamedataDamageType.Physical);
                        if stealthHitMult > 1.0 {
                            if isTough {
                                stealthHitMult *= 0.77;
                            }
                            hitEvent.attackComputed.MultAttackValue(stealthHitMult);
                        }
                    }
                }
            }
        }
    }
}

@replaceMethod(LocomotionTakedownDecisions)
protected const final func IsPowerLevelDifferentialTooHigh(target: wref<GameObject>) -> Bool {
    let isNgPlus = GameInstance.GetQuestsSystem(GetGameInstance()).GetFactStr("ngplus_active") == 1;
    let powDifference: gameEPowerDifferential = RPGManager.CalculatePowerDifferential(target);
    if Equals(powDifference, gameEPowerDifferential.IMPOSSIBLE) && !isNgPlus {
        return true;
    }

    return false;
}
*/