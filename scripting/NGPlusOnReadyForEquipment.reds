module NGPlus.PlayerProgression

import NGPlus.SpawnTags.NewGamePlusSpawnTagController

class PlayerProgressionLoader {
    private let m_ngPlusPlayerSaveData: PlayerSaveData;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_player: ref<PlayerPuppet>;
    private let m_equipmentSystem: ref<ScriptableSystem>;
    private let m_transactionSystem: ref<TransactionSystem>;
    private let m_isEp1: Bool;

    public final func LoadPlayerProgression(player: ref<PlayerPuppet>) -> Void {
        // Has some bugs during the Q001 start, iron them out later!
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        this.m_ngPlusPlayerSaveData = this.m_ngPlusSystem.GetSaveData();
        if !this.m_ngPlusPlayerSaveData.isValid {
            this.m_ngPlusSystem.Error("PlayerProgressionLoader::LoadPlayerProgression, save data invalid!");
            return;
        }
        this.m_player = player;
        this.m_isEp1 = IsEP1();

        this.m_transactionSystem = GameInstance.GetTransactionSystem(this.m_player.GetGame());

        this.LoadPlayerDevelopment();
        this.LoadPlayerInventory();
        this.LoadPlayerStash();
        this.LoadPlayerEquippedCyberware();
        this.LoadPlayerGarage();
        this.LoadPlayerCraftbook();
        this.LoadFacts();

        let playerDevelopmentData = PlayerDevelopmentSystem.GetInstance(player).GetDevelopmentData(player);
        playerDevelopmentData.ScaleItems();
        playerDevelopmentData.ScaleNPCsToPlayerLevel();

        NewGamePlusSpawnTagController.RestoreSpawnTags();

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerProgression done!");
    }

    private final func LoadFacts() {
        // This only exists because I really can't be arsed to alter fast tracking to Q001/Q101
        let questsSystem = GameInstance.GetQuestsSystem(this.m_player.GetGame());

        questsSystem.SetFactStr("sq032_johnny_friend", 1);
        questsSystem.SetFactStr("q003_jackie_motorcycle_upgraded", 1);
        questsSystem.SetFactStr("q000_patch_2_0_new_game", 1);

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadFacts done!");
    }   

    private final func LoadPlayerDevelopment() {
        let playerDevelopmentData: ref<PlayerDevelopmentData> = PlayerDevelopmentSystem.GetInstance(this.m_player).GetDevelopmentData(this.m_player);
        playerDevelopmentData.m_isInNgPlus = true;

        // FIX: unnecessary perk points getting added despite explicitly being set?
        let levelGainReason: telemetryLevelGainReason = telemetryLevelGainReason.Ignore;
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

        let targetLevel = this.m_ngPlusPlayerSaveData.playerLevel;

        if !IsEP1() {
            targetLevel = Min(targetLevel, 40); // FIX: broken progression on non-EP1 (This really should be in native, but I don't care...)
        }
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.Level,
                this.m_ngPlusPlayerSaveData.playerLevel,
                levelGainReason,
                true
            );
        playerDevelopmentData.SetDevelopmentsPoint(gamedataDevelopmentPointType.Attribute, this.m_ngPlusPlayerSaveData.playerAttributePoints);
        playerDevelopmentData
            .SetDevelopmentsPoint(
                gamedataDevelopmentPointType.Primary,
                this.m_ngPlusPlayerSaveData.playerPerkPoints
            );
                
        // Non-EP1 makes this look really wonky
        if this.m_isEp1 {
            playerDevelopmentData.SetDevelopmentsPoint(gamedataDevelopmentPointType.Espionage, this.m_ngPlusPlayerSaveData.playerRelicPoints);
            GameInstance.GetQuestsSystem(this.m_player.GetGame()).SetFact(n"ep1_tree_unlocked", 1);
        }
        
        playerDevelopmentData.m_isInNgPlus = false;
        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerDevelopment done!");
    }

    private final static func GetBigStatValue() -> Float {
        return 1000;
    }

    private final func LoadPlayerInventory() {
        let permaMod = RPGManager
            .CreateStatModifier(gamedataStatType.CarryCapacity, gameStatModifierType.Additive, PlayerProgressionLoader.GetBigStatValue());
        GameInstance
            .GetStatsSystem(this.m_player.GetGame())
            .AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);
        // Results in an annoying notification, I will not have that in MY Cyberpunk
        //  transactionSystem.GiveMoney(this.m_player, this.m_ngPlusPlayerSaveData.playerMoney, n"money");

        this.m_transactionSystem.GiveItem(this.m_player, ItemID.FromTDBID(t"Items.money"), this.m_ngPlusPlayerSaveData.playerMoney);

        for inventoryItem in this.m_ngPlusPlayerSaveData.playerItems {
            let modParams: ItemModParams;

            modParams.itemID = inventoryItem.itemId;
            modParams.quantity = inventoryItem.itemQuantity;
            modParams.customPartsToInstall = inventoryItem.attachments;

            let itemData = Inventory.CreateItemData(modParams, this.m_player);

            this.m_transactionSystem.GiveItemByItemData(this.m_player, itemData);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerInventory done!");
    }
    
    // Problem: by the time we call this function, the stash is not loaded in yet?
    // Wait for a few seconds before giving the player progression data, even though it'd be wacky?
    // Fixed by waiting for the node to be loaded before triggering the fact listener
    private final func GetPlayerStash() -> ref<GameObject> {
        let stashEntityId = Cast<EntityID>(
            ResolveNodeRef(
                CreateNodeRef("#v_room_stash"),
                Cast<GlobalNodeRef>(GlobalNodeID.GetRoot())
            )
        );

        let stashEntity = GameInstance.FindEntityByID(this.m_player.GetGame(), stashEntityId) as GameObject;
        return stashEntity;
    }

    private final func LoadPlayerStash() {
        let stashEntity = this.GetPlayerStash();
        for inventoryItem in this.m_ngPlusPlayerSaveData.playerStashItems {
            let modParams: ItemModParams;
            
            modParams.itemID = inventoryItem.itemId;
            modParams.quantity = inventoryItem.itemQuantity;

            for attachment in inventoryItem.attachments {
                let itemId = ItemID.DuplicateRandomSeedWithOffset(attachment, ItemID.GetTDBID(attachment), 0);

                ArrayPush(modParams.customPartsToInstall, itemId);
            }

            let itemData = Inventory.CreateItemData(modParams, this.m_player);

            this.m_transactionSystem.GiveItemByItemData(stashEntity, itemData);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerStash done!");
    }

    private final func EquipCyberware(item: ItemID, addToInventory: Bool, slotIndex: Int32) {
        if ItemID.IsValid(item) {
            let equipRequest = new EquipRequest();

            equipRequest.addToInventory = addToInventory;
            equipRequest.itemID = item;
            equipRequest.owner = this.m_player;
            equipRequest.slotIndex = slotIndex;

            this.m_equipmentSystem.QueueRequest(equipRequest);
        }
    }

    private final func LoadPlayerEquippedCyberware() {
        // TODO: calculate the needed value for equipped CW... Later, it's a bit wonky and I can't be arsed
        let permaMod = RPGManager
            .CreateStatModifier(gamedataStatType.Humanity, gameStatModifierType.Additive, PlayerProgressionLoader.GetBigStatValue());
        GameInstance
            .GetStatsSystem(this.m_player.GetGame())
            .AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);

        this.m_equipmentSystem = GameInstance.GetScriptableSystemsContainer(this.m_player.GetGame()).Get(n"EquipmentSystem");

        // In theory, we could just eat all the cyberware a player has - but I think that's kind of boring and gives more hassle with rebuilding
        // Also, cyberware capacity would fuck me over
        // Maybe a new character creation screen where the player can equip their own cyberware selection? Nah, pain in the ass

        // Might not get wanted CW (with proper stat modifiers) on start I think :(
        let osItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(this.m_ngPlusPlayerSaveData.playerEquippedOperatingSystem));
        let kiroshiItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(this.m_ngPlusPlayerSaveData.playerEquippedKiroshis));
        let armItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(this.m_ngPlusPlayerSaveData.playerEquippedArmCyberware));
        let legItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(this.m_ngPlusPlayerSaveData.playerEquippedLegCyberware));

        if IsDefined(osItemData) {
            this.EquipCyberware(osItemData.GetID(), false, -1);
        }

        if IsDefined(kiroshiItemData) {
            this.EquipCyberware(kiroshiItemData.GetID(), false, -1);
        }

        if IsDefined(armItemData) {
            this.EquipCyberware(armItemData.GetID(), false, -1);
        }

        if IsDefined(legItemData) {
            this.EquipCyberware(legItemData.GetID(), false, -1);
        }

        let i = 0;
        // This may fail with Cyberware-EX and other mods that fuck with equip slots?
        for cardiovascularCw in this.m_ngPlusPlayerSaveData.playerEquippedCardiacSystemCW {
            let itemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(cardiovascularCw));

            if IsDefined(itemData) {
                this.EquipCyberware(itemData.GetID(), false, i);
                i += 1;
            }
        }

        // After testing the Q001 start I've come to the conclusion Q001 is too much of a pain in the ass to do at Level 50 without armor
        // So we add a little bit
        let subdermalArmorId: ItemID;

        // Fix: Non-EP1 is wonky...
        if this.m_isEp1 {
            subdermalArmorId = ItemID.FromTDBID(t"Items.AdvancedBoringPlatingLegendaryPlusPlus");
        } else {
            subdermalArmorId = ItemID.FromTDBID(t"Items.AdvancedBoringPlatingLegendaryPlus");
        }

        this.EquipCyberware(subdermalArmorId, true, -1); // Might as well add it to inventory as well, free NG+ gift LMAO

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerEquippedCyberware done!");
    }

    private final func LoadPlayerGarage() {
        let vehicleSystem = GameInstance.GetVehicleSystem(this.m_player.GetGame());
        let vehicleList = this.m_ngPlusPlayerSaveData.playerVehicleGarage;

        let addedQuadra = false;
        let quadraTdbid = t"Vehicle.v_sport1_quadra_turbo_r_player";

        for vehicle in vehicleList {
            let record = TweakDBInterface.GetRecord(vehicle);

            // EP1 saves on non-EP1 game fix
            if IsDefined(record) {
                if !addedQuadra {
                    addedQuadra = Equals(vehicle, quadraTdbid);
                }
                vehicleSystem.EnablePlayerVehicleID(vehicle, true, false);
            }
        }

        if !addedQuadra {
            vehicleSystem.EnablePlayerVehicleID(quadraTdbid, true, false);
        }

        let adderSystem = GameInstance.GetScriptableSystemsContainer(this.m_player.GetGame()).Get(n"NGPlusVehicleAdderSystem") as NGPlusVehicleAdderSystem;

        if IsDefined(adderSystem) {
            this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerGarage, fixing Autofixer...");
            adderSystem.OnFinalizeVehicleAdditions();
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerGarage done!");
    }

    private final func LoadPlayerCraftbook() {
        let craftingSystem = CraftingSystem.GetInstance(this.m_player.GetGame());

        if !IsDefined(craftingSystem) {
            this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerCraftbook failed, craftingSystem == NULL");
            return;
        }

        let craftBook = craftingSystem.GetPlayerCraftBook();

        for targetItem in this.m_ngPlusPlayerSaveData.knownRecipeTargetItems {
            craftBook.AddRecipeFromInfo(targetItem);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerCraftbook done!");
    }
}

class NewGamePlusProgressionLoader extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_progressionLoaderListenerId: Uint32;

    private final func OnAttach() -> Void {
        this.m_questsSystem = GameInstance.GetQuestsSystem(GetGameInstance());
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        
        this.m_progressionLoaderListenerId = this.m_questsSystem.RegisterListener(n"ngplus_apply_progression", this, n"OnProgressionTransferCalled");
    }

    public final func OnProgressionTransferCalled(factValue: Int32) -> Void {
        this.m_ngPlusSystem.Spew(s"NewGamePlusProgressionLoader::OnProgressionTransferCalled \(factValue)");
        if(factValue != 1) {
            return;
        }

        let player = GameInstance.GetPlayerSystem(GetGameInstance()).GetLocalPlayerMainGameObject() as PlayerPuppet;

        if !IsDefined(player) {
            this.m_questsSystem.SetFactStr("ngplus_apply_progression", 0);
            this.m_ngPlusSystem.Error("NewGamePlusProgressionLoader::OnProgressionTransferCalled, player not found");
            return;
        }

        let loader = new PlayerProgressionLoader();

        loader.LoadPlayerProgression(player);

        this.m_questsSystem.SetFactStr("ngplus_apply_progression", 0);
    }

    private final func OnDetach() -> Void {
        this.m_questsSystem.UnregisterListener(n"ngplus_apply_progression", this.m_progressionLoaderListenerId);
    }
}