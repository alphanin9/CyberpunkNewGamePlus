module NGPlus.PlayerProgression

import NGPlus.SpawnTags.NewGamePlusSpawnTagController
import NGPlus.EP1Listener.NGPlusEP1StatusListener
import NGPlus.Ripperdoc.NGPlusTutorialCyberwareProvider

@if(ModuleExists("EquipmentEx"))
import EquipmentEx.*

class PlayerProgressionLoader {
    private let m_ngPlusProgression: ref<NGPlusProgressionData>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_player: ref<PlayerPuppet>;
    private let m_equipmentSystem: ref<ScriptableSystem>;
    private let m_transactionSystem: ref<TransactionSystem>;
    private let m_statsSystem: ref<StatsSystem>;
    private let m_delaySystem: ref<DelaySystem>;
    private let m_isEp1: Bool;

    public final func LoadPlayerProgression(player: ref<PlayerPuppet>) -> Void {
        // Has some bugs during the Q001 start, iron them out later!
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        this.m_ngPlusProgression = this.m_ngPlusSystem.GetProgressionData();

        if !IsDefined(this.m_ngPlusProgression) {
            this.m_ngPlusSystem.Error("PlayerProgressionLoader::LoadPlayerProgression, save data invalid!");
            return;
        }
        
        this.m_player = player;
        this.m_isEp1 = IsEP1();

        this.m_transactionSystem = GameInstance.GetTransactionSystem(this.m_player.GetGame());
        this.m_statsSystem = GameInstance.GetStatsSystem(this.m_player.GetGame());
        this.m_delaySystem = GameInstance.GetDelaySystem(this.m_player.GetGame());
        this.m_equipmentSystem = GameInstance.GetScriptableSystemsContainer(this.m_player.GetGame()).Get(n"EquipmentSystem");

        let questsSystem = GameInstance.GetQuestsSystem(this.m_player.GetGame());

        if questsSystem.GetFactStr("ngplus_q001_start") == 1 {
            // Equip V's clothes...
            // NOTE: this might double add them? Fix it in questphase... (probably)
            
            let shirtId = ItemID.FromTDBID(t"Items.Q001_TShirt");
            let pantsId = ItemID.FromTDBID(t"Items.Q001_Pants");
            let shoesId = ItemID.FromTDBID(t"Items.Q001_Shoes");

            this.EquipCyberware(shirtId, true, 0);
            this.EquipCyberware(pantsId, true, 0);
            this.EquipCyberware(shoesId, true, 0);
        }

        this.LoadPlayerDevelopment();
        this.LoadPlayerInventory();
        this.LoadPlayerStash();
        this.LoadPlayerEquippedCyberware();
        this.LoadPlayerGarage();
        this.LoadPlayerCraftbook();
        this.LoadPlayerWardrobe();
        this.LoadEquipmentEx();
        this.LoadFacts();

        let playerDevelopmentData = PlayerDevelopmentSystem.GetInstance(player).GetDevelopmentData(player);
        playerDevelopmentData.ScaleNPCsToPlayerLevel();

        NewGamePlusSpawnTagController.RestoreSpawnTags();
        NGPlusEP1StatusListener.ApplyRandomEncounterDisabler(questsSystem);

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

        let statsSystemResults = this.m_ngPlusProgression.GetStatsSystemResults();

        // FIX: unnecessary perk points getting added despite explicitly being set?
        let levelGainReason: telemetryLevelGainReason = telemetryLevelGainReason.Ignore;
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.StrengthSkill,
                Cast<Int32>(statsSystemResults.GetBodySkill()),
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.ReflexesSkill,
                Cast<Int32>(statsSystemResults.GetReflexesSkill()),
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.TechnicalAbilitySkill,
                Cast<Int32>(statsSystemResults.GetTechnicalAbilitySkill()),
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.IntelligenceSkill,
                Cast<Int32>(statsSystemResults.GetIntelligenceSkill()),
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.CoolSkill,
                Cast<Int32>(statsSystemResults.GetCoolSkill()),
                levelGainReason,
                true
            );
        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.StreetCred,
                Cast<Int32>(statsSystemResults.GetStreetCred()),
                levelGainReason,
                true
            );

        playerDevelopmentData
            .SetLevel(
                gamedataProficiencyType.Level,
                Cast<Int32>(statsSystemResults.GetLevel()),
                levelGainReason,
                true
            );

        let playerDevelopmentResults = this.m_ngPlusProgression.GetPlayerDevelopmentSystemResults();
        
        playerDevelopmentData.SetDevelopmentsPoint(gamedataDevelopmentPointType.Attribute, playerDevelopmentResults.GetAttributePoints());
        playerDevelopmentData
            .SetDevelopmentsPoint(
                gamedataDevelopmentPointType.Primary,
                playerDevelopmentResults.GetPerkPoints()
            );
                
        // Non-EP1 makes this look really wonky
        if this.m_isEp1 {
            playerDevelopmentData.SetDevelopmentsPoint(gamedataDevelopmentPointType.Espionage, playerDevelopmentResults.GetRelicPoints());
            GameInstance.GetQuestsSystem(this.m_player.GetGame()).SetFact(n"ep1_tree_unlocked", 1);
        }
        
        playerDevelopmentData.m_isInNgPlus = false;
        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerDevelopment done!");
    }

    private final static func GetBigStatValue() -> Float {
        return 1000;
    }
    
    public final func ApplyStatModifiers(item: ref<NGPlusItemData>, objId: StatsObjectID) {
        let itemQuality = 0.0;
        let itemUpgradeCount = 0.0;

        let statModifiers = item.GetStatModifiers();

        // Maybe do this on the native side? ...
        for modifier in statModifiers {
            if Equals(modifier.statType, gamedataStatType.Quality) {
                let asConstant = modifier as gameConstantStatModifierData;
                if IsDefined(asConstant) {
                    if asConstant.value < 0.0 {
                        itemQuality -= asConstant.value;
                    } else {
                        itemQuality += asConstant.value;
                    }
                }
            }

            if Equals(modifier.statType, gamedataStatType.WasItemUpgraded) {
                let asConstant = modifier as gameConstantStatModifierData;
                if IsDefined(asConstant) {
                    itemUpgradeCount += asConstant.value;
                }
            }
        }
    	
        let scalingBlocked = RPGManager.CreateStatModifier(gamedataStatType.ScalingBlocked, gameStatModifierType.Additive, 1);
        let qualityModifier = RPGManager.CreateStatModifier(gamedataStatType.Quality, gameStatModifierType.Additive, itemQuality);
        let upgradeModifier = RPGManager.CreateStatModifier(gamedataStatType.WasItemUpgraded, gameStatModifierType.Additive, itemUpgradeCount);

        this.m_statsSystem.AddSavedModifier(objId, qualityModifier);
        this.m_statsSystem.AddSavedModifier(objId, upgradeModifier);
        this.m_statsSystem.AddSavedModifier(objId, scalingBlocked);

        for modifier in statModifiers {
            // NOTE: this feels very hacky - and it is hacky as shit...
            // The item quality/upgrade system is completely fucked, and I dislike it
            if NotEquals(modifier.statType, gamedataStatType.WasItemUpgraded) && NotEquals(modifier.statType, gamedataStatType.Quality) && NotEquals(modifier.statType, gamedataStatType.Invalid) {
                let asConstant = modifier as gameConstantStatModifierData;
                if IsDefined(asConstant) {
                    let newModifier = RPGManager.CreateStatModifier(asConstant.statType, asConstant.modifierType, asConstant.value);
                    this.m_statsSystem.AddSavedModifier(objId, newModifier);
                }

                let asCombined = modifier as gameCombinedStatModifierData;

                if IsDefined(asCombined) {
                    let newModifier = RPGManager.CreateCombinedStatModifier(asCombined.statType, asCombined.modifierType, asCombined.refStatType, asCombined.operation, asCombined.value, asCombined.refObject);
                    this.m_statsSystem.AddSavedModifier(objId, newModifier);
                }

                let asCurve = modifier as gameCurveStatModifierData;

                if IsDefined(asCurve) {
                    let newModifier = RPGManager.CreateStatModifierUsingCurve(asCurve.statType, asCurve.modifierType, asCurve.curveStat, asCurve.curveName, asCurve.columnName);
                    this.m_statsSystem.AddSavedModifier(objId, newModifier);
                }
            }
        }
    }

    public final func AddItemToInventory(item: ref<NGPlusItemData>, target: ref<GameObject>, addStats: Bool) -> Bool {
        addStats = addStats && Equals(item.GetItemId().GetStructure(), gamedataItemStructure.Unique);

        let itemQuantity = Max(1, item.GetItemQuantity());
        let itemAttachments = item.GetAttachments();

        if ArraySize(itemAttachments) > 0 {
            let modParams: ItemModParams;

            modParams.itemID = item.GetItemId();
            modParams.quantity = itemQuantity;
            modParams.customPartsToInstall = itemAttachments;

            let itemData = Inventory.CreateItemData(modParams, target);

            if !this.m_transactionSystem.GiveItemByItemData(target, itemData) {
                return false;
            }

            if addStats {
                this.ApplyStatModifiers(item, itemData.GetStatsObjectID());
            }
            
            return true;
        }

        if !this.m_transactionSystem.GiveItem(target, item.GetItemId(), itemQuantity) {
            return false;
        }

        if addStats {
            this.ApplyStatModifiers(item, Cast<StatsObjectID>(item.GetItemId()));
        }

        return true;
    }

    private final func LoadPlayerStash() {
        // Stash should always be loaded in by the first scene now
        let stashId = Cast<EntityID>(ResolveNodeRef(CreateNodeRef("#v_room_stash"), Cast<GlobalNodeRef>(GlobalNodeID.GetRoot())));
        let stashEntity = GameInstance.FindEntityByID(GetGameInstance(), stashId) as GameObject;

        if !IsDefined(stashEntity) {
            this.m_ngPlusSystem.Error("Stash entity was not loaded after all!");
            return;
        }

        for stashItem in this.m_ngPlusProgression.GetPlayerInventory().GetStash() {
            this.AddItemToInventory(stashItem, stashEntity, true);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerStash done!");
    }

    private final func LoadPlayerInventory() {
        // Note: is there a point to transferring carry capacity? Cyberware cap has a point - Edgerunner can be a part of a build...
        let permaMod = RPGManager
            .CreateStatModifier(gamedataStatType.CarryCapacity, gameStatModifierType.Additive, PlayerProgressionLoader.GetBigStatValue());
        GameInstance
            .GetStatsSystem(this.m_player.GetGame())
            .AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);

        let inventory = this.m_ngPlusProgression.GetPlayerInventory();

        this.m_transactionSystem.GiveItem(this.m_player, ItemID.FromTDBID(t"Items.money"), inventory.GetMoney());

        for inventoryItem in inventory.GetInventory() {
            this.AddItemToInventory(inventoryItem, this.m_player, true);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerInventory done!");
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
        let statsSystemResults = this.m_ngPlusProgression.GetStatsSystemResults();
        
        for cyberwareAddedModifier in statsSystemResults.GetCyberwareCapacity() {
            let permaMod = RPGManager.CreateStatModifier(gamedataStatType.Humanity, gameStatModifierType.Additive, cyberwareAddedModifier);
            this.m_statsSystem.AddSavedModifier(Cast<StatsObjectID>(this.m_player.GetEntityID()), permaMod);
        }

        let equipmentSystemResults = this.m_ngPlusProgression.GetEquipmentSystemResults();

        let osItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(equipmentSystemResults.GetPlayerEquippedOperatingSystem()));
        let kiroshiItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(equipmentSystemResults.GetPlayerEquippedKiroshis()));
        let armItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(equipmentSystemResults.GetPlayerEquippedArmCyberware()));
        let legItemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(equipmentSystemResults.GetPlayerEquippedLegCyberware()));

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
        for cardiovascularCw in equipmentSystemResults.GetPlayerEquippedCardiacSystemCW() {
            let itemData = this.m_transactionSystem.GetItemDataByTDBID(this.m_player, ItemID.GetTDBID(cardiovascularCw));

            if IsDefined(itemData) {
                this.EquipCyberware(itemData.GetID(), false, i);
                i += 1;
            }
        }

        // After testing the Q001 start I've come to the conclusion Q001 is too much of a pain in the ass to do at Level 50 without armor
        // So we add a little bit
        

        let tutorialItemQuality = RPGManager.ConvertPlayerLevelToCyberwareQuality(GameInstance.GetStatsSystem(this.m_player.GetGame()).GetStatValue(Cast<StatsObjectID>(this.m_player.GetEntityID()), gamedataStatType.Level), false);
        let subdermalArmorId = ItemID.FromTDBID(NGPlusTutorialCyberwareProvider.GetArmorCyberware(tutorialItemQuality));

        this.EquipCyberware(subdermalArmorId, true, -1); // Might as well add it to inventory as well, free NG+ gift LMAO

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerEquippedCyberware done!");
    }

    private final func LoadPlayerGarage() {
        let vehicleSystem = GameInstance.GetVehicleSystem(this.m_player.GetGame());
        let vehicleList = this.m_ngPlusProgression.GetVehicleGarageResults().GetGarage();

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
        let craftInfo = this.m_ngPlusProgression.GetCraftingSystemResults().GetData();
        for targetItem in craftInfo {
            craftBook.AddRecipeFromInfo(targetItem);
        }

        this.m_ngPlusSystem.Spew("PlayerProgressionLoader::LoadPlayerCraftbook done!");
    }

    private final func LoadPlayerWardrobe() {
        let wardrobeSystem = GameInstance.GetWardrobeSystem(this.m_player.GetGame());

        for entry in this.m_ngPlusProgression.GetWardrobeResults().GetWardrobe() {
            wardrobeSystem.StoreUniqueItemID(entry);
        }
    }

    @if(ModuleExists("EquipmentEx"))
    private final func LoadEquipmentEx() {
        let viewManagerResults = this.m_ngPlusProgression.GetEquipmentExViewManagerResults();

        if IsDefined(viewManagerResults) {
            let itemSource = viewManagerResults.GetSource();
            let viewManager = ViewManager.GetInstance(GetGameInstance());

            switch(itemSource) {
                case 0ul:
                    viewManager.SetItemSource(WardrobeItemSource.WardrobeStore);
                    break;
                case 1ul:
                    viewManager.SetItemSource(WardrobeItemSource.InventoryAndStash);
                    break;
                case 2ul:
                    viewManager.SetItemSource(WardrobeItemSource.InventoryOnly);
                    break;
                default:
                    break;
            }
        }

        let outfits = this.m_ngPlusProgression.GetEquipmentExOutfitSystemResults();

        if IsDefined(outfits) {
            let outfitSystem = OutfitSystem.GetInstance(GetGameInstance());

            for outfit in outfits.GetData() {
                let outfitParts: [ref<OutfitPart>];

                for outfitPart in outfit.GetOutfitParts() {
                    ArrayPush(outfitParts, OutfitPart.Create(outfitPart.GetItemID(), outfitPart.GetSlotID()));    
                }

                outfitSystem.AddOutfit(outfit.GetName(), outfitParts, false);
            }
        }
    }

    @if(!ModuleExists("EquipmentEx"))
    private final func LoadEquipmentEx() {
        // N/A
    }
}

class NewGamePlusProgressionLoader extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_transactionSystem: ref<TransactionSystem>;
    private let m_progressionLoaderListenerId: Uint32;

    private let m_stashEntity: ref<GameObject>;

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
    }

    private final func OnDetach() -> Void {
        this.m_questsSystem.UnregisterListener(n"ngplus_apply_progression", this.m_progressionLoaderListenerId);
    }
}