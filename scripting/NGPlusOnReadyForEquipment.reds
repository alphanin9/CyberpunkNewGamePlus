module NGPlus.PlayerProgression

import NGPlus.SpawnTags.NewGamePlusSpawnTagController

class PlayerProgressionLoader {
    private let m_ngPlusPlayerSaveData: PlayerSaveData;
    private let m_player: ref<PlayerPuppet>;
    private let m_equipmentSystem: ref<ScriptableSystem>;

    public func LoadPlayerProgression(player: ref<PlayerPuppet>) -> Void {
        // Has some bugs during the Q001 start, iron them out later!
        this.m_ngPlusPlayerSaveData = GameInstance.GetNewGamePlusSystem().GetSaveData();
        if !this.m_ngPlusPlayerSaveData.isValid {
            return;
        }
        this.m_player = player;

        this.LoadPlayerDevelopment();
        this.LoadPlayerInventory();
        this.LoadPlayerStash();
        this.LoadPlayerEquippedCyberware();
        this.LoadPlayerGarage();
        this.LoadFacts();

        let playerDevelopmentData = PlayerDevelopmentSystem.GetInstance(player).GetDevelopmentData(player);
        playerDevelopmentData.ScaleItems();
        playerDevelopmentData.ScaleNPCsToPlayerLevel();

        NewGamePlusSpawnTagController.RestoreSpawnTags();
    }

    private func LoadFacts() {
        // This only exists because I really can't be arsed to alter fast tracking to Q001/Q101
        let questsSystem = GameInstance.GetQuestsSystem(this.m_player.GetGame());

        questsSystem.SetFactStr("sq032_johnny_friend", 1);
        questsSystem.SetFactStr("q003_jackie_motorcycle_upgraded", 1);
        questsSystem.SetFactStr("q000_patch_2_0_new_game", 1);
    }   

    private func LoadPlayerDevelopment() {
        let playerDevelopmentData: ref<PlayerDevelopmentData> = PlayerDevelopmentSystem.GetInstance(this.m_player).GetDevelopmentData(this.m_player);
        playerDevelopmentData.m_isInNgPlus = true;
        let levelGainReason: telemetryLevelGainReason = telemetryLevelGainReason.IsDebug;
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.StrengthSkill,
                this.m_ngPlusPlayerSaveData.playerBodySkillLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.ReflexesSkill,
                this.m_ngPlusPlayerSaveData.playerReflexSkillLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.TechnicalAbilitySkill,
                this.m_ngPlusPlayerSaveData.playerTechSkillLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.IntelligenceSkill,
                this.m_ngPlusPlayerSaveData.playerIntelligenceSkillLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.CoolSkill,
                this.m_ngPlusPlayerSaveData.playerCoolSkillLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.StreetCred,
                this.m_ngPlusPlayerSaveData.playerStreetCred,
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.Level,
                this.m_ngPlusPlayerSaveData.playerLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData.SetDevelopmentsPoint(gamedataDevelopmentPointType.Attribute, 0);
        playerDevelopmentData
            .SetDevelopmentsPoint(
                gamedataDevelopmentPointType.Primary,
                this.m_ngPlusPlayerSaveData.playerPerkPoints
            );
        playerDevelopmentData
            .SetDevelopmentsPoint(
                gamedataDevelopmentPointType.Espionage,
                this.m_ngPlusPlayerSaveData.playerRelicPoints
            );
        GameInstance
            .GetQuestsSystem(this.m_player.GetGame())
            .SetFact(n"ep1_tree_unlocked", 1);
        playerDevelopmentData.m_isInNgPlus = false;
    }

    private final static func GetBigStatValue() -> Float {
        return 1000;
    }

    private func LoadPlayerInventory() {
        let permaMod = RPGManager
            .CreateStatModifier(gamedataStatType.CarryCapacity, gameStatModifierType.Additive, PlayerProgressionLoader.GetBigStatValue());
        GameInstance
            .GetStatsSystem(this.m_player.GetGame())
            .AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);
        let transactionSystem: ref<TransactionSystem> = GameInstance.GetTransactionSystem(this.m_player.GetGame());
        // Results in an annoying notification, I will not have that in MY Cyberpunk
        //  transactionSystem.GiveMoney(this.m_player, this.m_ngPlusPlayerSaveData.playerMoney, n"money");
        
        transactionSystem.GiveItem(this.m_player, ItemID.FromTDBID(t"Items.money"), this.m_ngPlusPlayerSaveData.playerMoney);

        for inventoryItem in this.m_ngPlusPlayerSaveData.playerItems {
            transactionSystem
                .GiveItem(this.m_player, inventoryItem.itemId, inventoryItem.itemQuantity);
        }
    }
    
    // Problem: by the time we call this function, the stash is not loaded in yet?
    // Wait for a few seconds before giving the player progression data, even though it'd be wacky?
    private func GetPlayerStash() -> ref<GameObject> {
        let stashEntityId = Cast<EntityID>(
            ResolveNodeRef(
                CreateNodeRef("#v_room_stash"),
                Cast<GlobalNodeRef>(GlobalNodeID.GetRoot())
            )
        );

        let stashEntity = GameInstance.FindEntityByID(this.m_player.GetGame(), stashEntityId) as GameObject;
        return stashEntity;
    }

    private func LoadPlayerStash() {
        let transactionSystem: ref<TransactionSystem> = GameInstance.GetTransactionSystem(this.m_player.GetGame());
        let stashEntity = this.GetPlayerStash();
        for inventoryItem in this.m_ngPlusPlayerSaveData.playerStashItems {
            transactionSystem
                .GiveItem(stashEntity, inventoryItem.itemId, inventoryItem.itemQuantity);
        }
    }

    private func EquipCyberware(item: ItemID, addToInventory: Bool) {
        if ItemID.IsValid(item) {
            let equipRequest = new EquipRequest();

            equipRequest.addToInventory = addToInventory;
            equipRequest.itemID = item;
            equipRequest.owner = this.m_player;
            equipRequest.slotIndex = 0;

            this.m_equipmentSystem.QueueRequest(equipRequest);
        }
    }

    private func LoadPlayerEquippedCyberware() {
        let permaMod = RPGManager
            .CreateStatModifier(gamedataStatType.Humanity, gameStatModifierType.Additive, PlayerProgressionLoader.GetBigStatValue());
        GameInstance
            .GetStatsSystem(this.m_player.GetGame())
            .AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);

        this.m_equipmentSystem = GameInstance.GetScriptableSystemsContainer(this.m_player.GetGame()).Get(n"EquipmentSystem");

        // In theory, we could just eat all the cyberware a player has - but I think that's kind of boring and gives more hassle with rebuilding
        // Also, cyberware capacity would fuck me over
        // Maybe a new character creation screen where the player can equip their own cyberware selection? Nah, pain in the ass

        this.EquipCyberware(this.m_ngPlusPlayerSaveData.playerEquippedOperatingSystem, false);
        this.EquipCyberware(this.m_ngPlusPlayerSaveData.playerEquippedKiroshis, false);
        this.EquipCyberware(this.m_ngPlusPlayerSaveData.playerEquippedArmCyberware, false);
        this.EquipCyberware(this.m_ngPlusPlayerSaveData.playerEquippedLegCyberware, false);

        // After testing the Q001 start I've come to the conclusion Q001 is too much of a pain in the ass to do at Level 50 without armor
        // So we add a little bit
        let subdermalArmorId = ItemID.FromTDBID(t"Items.AdvancedBoringPlatingLegendaryPlusPlus");

        this.EquipCyberware(subdermalArmorId, true); // Might as well add it to inventory as well, free NG+ gift LMAO
    }

    private func LoadPlayerGarage() {
        let vehicleSystem = GameInstance.GetVehicleSystem(this.m_player.GetGame());
        let vehicleList = this.m_ngPlusPlayerSaveData.playerVehicleGarage;

        let addedQuadra = false;
        let quadraTdbid = t"Vehicle.v_sport1_quadra_turbo_r_player";

        for vehicle in vehicleList {
            if !addedQuadra {
                addedQuadra = Equals(vehicle, quadraTdbid);
            }
            vehicleSystem.EnablePlayerVehicleID(vehicle, true, false);
        }

        if !addedQuadra {
            vehicleSystem.EnablePlayerVehicleID(quadraTdbid, true, false);
        }
    }
}

@addField(PlayerPuppet)
let m_hasAppliedSavegameProgression: Bool;

@addMethod(PlayerPuppet)
protected cb func OnNewGamePlusLoadProgressionEvent(event: ref<ActionEvent>) {
    if NotEquals(event.eventAction, n"OnProgressionLoading") {
        return;
    }
    if this.m_hasAppliedSavegameProgression {
        return;
    }
    this.m_hasAppliedSavegameProgression = true;
    let progressionLoader = new PlayerProgressionLoader();
    progressionLoader.LoadPlayerProgression(this);
}