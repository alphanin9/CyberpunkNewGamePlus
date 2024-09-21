
// Scanner gets fucked with huge ability counts, and random encounter bosses have flavor text now...
@replaceMethod(ScannerAbilitiesGameController)
protected cb func OnAbilitiesChanged(value: Variant) -> Bool {
    let abilitiesList: [wref<GameplayAbility_Record>];
    let abilityData: ref<AbilityUserData>;
    let abilityStruct: ref<GameplayAbility_Record>;
    let asyncSpawnRequest: wref<inkAsyncSpawnRequest>;
    let i: Int32;
    let limit: Int32;
    let abilitiesData: ref<ScannerAbilities> = FromVariant<ref<ScannerAbilities>>(value);
    this.ClearAllAsyncSpawnRequests();
    if IsDefined(abilitiesData) {
        abilitiesList = abilitiesData.GetAbilities();

        let isNgPlusBoss = false;

        for ability in abilitiesList {
            if Equals(ability.GetRecordID(), t"Ability.NGPlus_RandomEncounterBossMarker") {
                isNgPlusBoss = true;
                break;        
            }
        }

        if !isNgPlusBoss {
            limit = ArraySize(abilitiesList);
            i = 0;
            while i < limit {
                abilityStruct = abilitiesList[i];
                if abilityStruct.ShowInCodex() {
                    abilityData = new AbilityUserData();
                    abilityData.abilityID = abilityStruct.GetID();
                    abilityData.locKeyName = abilityStruct.Loc_key_name();
                    asyncSpawnRequest = this
                        .AsyncSpawnFromLocal(
                            inkWidgetRef.Get(this.m_ScannerAbilitiesRightPanel),
                            n"ScannerAbilityItemWidget",
                            this,
                            n"OnAbilitySpawned",
                            abilityData
                        );
                    abilityData.asyncSpawnRequest = asyncSpawnRequest;
                    ArrayPush(this.m_asyncSpawnRequests, asyncSpawnRequest);
                }
                i += 1;
            }
            this.m_isValidAbilities = true;
        } else {
            this.m_isValidAbilities = false;
        }
    } else {
        this.m_isValidAbilities = false;
    }
    this.UpdateGlobalVisibility();
}

@wrapMethod(ScannerBountySystemGameController)
protected cb func OnBountySystemChanged(value: Variant) -> Bool {
    let projectGhostTransgression = TweakDBInterface.GetString(t"Transgression.ProjectGhostMember.localizedDescription", "");

    let bounty = FromVariant<ref<ScannerBountySystem>>(value);

    let bountyData = bounty.GetBounty();

    for transgression in bountyData.transgressions {
        if Equals(projectGhostTransgression, transgression) {
            bountyData.level = 5;
            bounty.Set(bountyData);
            
            break;
        }
    }

    return wrappedMethod(value);
}