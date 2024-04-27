module NGPlus.Ripperdoc

public class NGPlusTutorialCyberwareProvider {
    public static func GetEyesCyberware(itemQuality: gamedataQuality) -> TweakDBID {
        switch itemQuality {
        case gamedataQuality.Common:
            return t"Items.AdvancedKiroshiOpticsBareCommon";
        case gamedataQuality.CommonPlus:
            return t"Items.AdvancedKiroshiOpticsBareCommonPlus";
        case gamedataQuality.Uncommon:
            return t"Items.AdvancedKiroshiOpticsBareUncommon";
        case gamedataQuality.UncommonPlus:
            return t"Items.AdvancedKiroshiOpticsBareUncommonPlus";
        case gamedataQuality.Rare:
            return t"Items.AdvancedKiroshiOpticsBareRare";
        case gamedataQuality.RarePlus:
            return t"Items.AdvancedKiroshiOpticsBareRarePlus";
        case gamedataQuality.Epic:
            return t"Items.AdvancedKiroshiOpticsBareEpic";
        case gamedataQuality.EpicPlus:
            return t"Items.AdvancedKiroshiOpticsBareEpicPlus";
        case gamedataQuality.Legendary:
            return t"Items.AdvancedKiroshiOpticsBareLegendary";
        case gamedataQuality.LegendaryPlus:
            return t"Items.AdvancedKiroshiOpticsBareLegendaryPlus";
        case gamedataQuality.LegendaryPlusPlus:
            return t"Items.AdvancedKiroshiOpticsBareLegendaryPlusPlus";
        default:
            return t"Items.AdvancedKiroshiOptics_q001_1";
        };
    }

    public static func GetHandsCyberware(itemQuality: gamedataQuality) -> TweakDBID {
        switch itemQuality {
        case gamedataQuality.Common:
            return t"Items.AdvancedPowerGripCommon";
        case gamedataQuality.CommonPlus:
            return t"Items.AdvancedPowerGripCommonPlus";
        case gamedataQuality.Uncommon:
            return t"Items.AdvancedPowerGripUncommon";
        case gamedataQuality.UncommonPlus:
            return t"Items.AdvancedPowerGripUncommonPlus";
        case gamedataQuality.Rare:
            return t"Items.AdvancedPowerGripRare";
        case gamedataQuality.RarePlus:
            return t"Items.AdvancedPowerGripRarePlus";
        case gamedataQuality.Epic:
            return t"Items.AdvancedPowerGripEpic";
        case gamedataQuality.EpicPlus:
            return t"Items.AdvancedPowerGripEpicPlus";
        case gamedataQuality.Legendary:
            return t"Items.AdvancedPowerGripLegendary";
        case gamedataQuality.LegendaryPlus:
            return t"Items.AdvancedPowerGripLegendaryPlus";
        case gamedataQuality.LegendaryPlusPlus:
            return t"Items.AdvancedPowerGripLegendaryPlusPlus";
        default:
            return t"Items.AdvancedPowerGrip_q001_1";
        };
    }

    public static func GetArmorCyberware(itemQuality: gamedataQuality) -> TweakDBID {
        switch itemQuality {
        case gamedataQuality.Common:
            return t"Items.AdvancedBoringPlatingCommon";
        case gamedataQuality.CommonPlus:
            return t"Items.AdvancedBoringPlatingCommonPlus";
        case gamedataQuality.Uncommon:
            return t"Items.AdvancedBoringPlatingUncommon";
        case gamedataQuality.UncommonPlus:
            return t"Items.AdvancedBoringPlatingUncommonPlus";
        case gamedataQuality.Rare:
            return t"Items.AdvancedBoringPlatingRare";
        case gamedataQuality.RarePlus:
            return t"Items.AdvancedBoringPlatingRarePlus";
        case gamedataQuality.Epic:
            return t"Items.AdvancedBoringPlatingEpic";
        case gamedataQuality.EpicPlus:
            return t"Items.AdvancedBoringPlatingEpicPlus";
        case gamedataQuality.Legendary:
            return t"Items.AdvancedBoringPlatingLegendary";
        case gamedataQuality.LegendaryPlus:
            return t"Items.AdvancedBoringPlatingLegendaryPlus";
        case gamedataQuality.LegendaryPlusPlus:
            return t"Items.AdvancedBoringPlatingLegendaryPlusPlus";
        default:
            return t"Items.AdvancedBoringPlating_Q001";
        };
    }
}

@addMethod(RipperDocGameController)
private final func AddTutorialItemsToVendorNgPlus() {
    // We kinda fuck up and don't let Viktor add his tutorial items (subdermal armor, the like), which is a *problem* - we don't get subdermal armor from Vik
    // Should fix it

    if this.m_questSystem.GetFact(n"tutorial_ripperdoc_items_added") > 0 {
        return;
    }

    let tutorialItemQuality = RPGManager.ConvertPlayerLevelToCyberwareQuality(GameInstance.GetStatsSystem(this.m_player.GetGame()).GetStatValue(Cast<StatsObjectID>(this.m_playerID), gamedataStatType.Level), false);

    this.m_tutorialEyesCW = NGPlusTutorialCyberwareProvider.GetEyesCyberware(tutorialItemQuality);
    this.m_tutorialHandsCW = NGPlusTutorialCyberwareProvider.GetHandsCyberware(tutorialItemQuality);
    this.m_tutorialArmorCW = NGPlusTutorialCyberwareProvider.GetArmorCyberware(tutorialItemQuality);

    let vendor = MarketSystem.GetInstance(this.m_VendorDataManager.GetVendorInstance().GetGame()).GetVendor(this.m_VendorDataManager.GetVendorInstance());
    let transactionSystem = GameInstance.GetTransactionSystem(this.m_player.GetGame());

    let itemStack: SItemStack;
    itemStack.quantity = 1;

    let eyesItem = ItemID.FromTDBID(this.m_tutorialEyesCW);
    let handsItem = ItemID.FromTDBID(this.m_tutorialHandsCW);
    let armorItem = ItemID.FromTDBID(this.m_tutorialArmorCW);

    if !transactionSystem.HasItem(vendor.GetVendorObject(), eyesItem) {
        itemStack.itemID = eyesItem;
        vendor.AddItemsToStock(itemStack);
        transactionSystem.GiveItem(vendor.GetVendorObject(), itemStack.itemID, itemStack.quantity);
    }

    if !transactionSystem.HasItem(vendor.GetVendorObject(), handsItem) {
        itemStack.itemID = handsItem;
        vendor.AddItemsToStock(itemStack);
        transactionSystem.GiveItem(vendor.GetVendorObject(), itemStack.itemID, itemStack.quantity);
    }

    if !transactionSystem.HasItem(vendor.GetVendorObject(), armorItem) {
        itemStack.itemID = armorItem;
        vendor.AddItemsToStock(itemStack);
        transactionSystem.GiveItem(vendor.GetVendorObject(), itemStack.itemID, itemStack.quantity);
    }

    LogChannel(n"DEBUG", "Added tutorial cyberware!");

    this.m_questSystem.SetFact(n"tutorial_ripperdoc_items_added", 1);
}

@replaceMethod(RipperDocGameController)
protected cb func OnInitialize() -> Bool {
    let currentAllocatedCapacity: Float;
    let freedCapacity: Float;
    let i: Int32;
    let names: array<String>;
    let requiredCapacity: Float;
    let tutorialItemQuality: gamedataQuality;
    let vendorData: VendorData;
    let vendorPanelData: ref<VendorPanelData>;
    inkWidgetRef.SetVisible(this.m_inventoryWarnning, false);
    this.m_inventoryView = inkWidgetRef.GetController(this.m_inventoryViewAnchor) as RipperdocInventoryController;
    this.m_selector = inkWidgetRef.GetController(this.m_selectorAnchor) as RipperdocSelectorController;
    this.m_player = this.GetPlayerControlledObject() as PlayerPuppet;
    this.m_playerID = this.m_player.GetEntityID();
    this.m_audioSystem = GameInstance.GetAudioSystem(this.m_player.GetGame());
    this.m_uiSystem = GameInstance.GetUISystem(this.m_player.GetGame());
    this.m_questSystem = GameInstance.GetQuestsSystem(this.m_player.GetGame());
    this.m_filterMode = RipperdocModes.Default;
    if IsDefined(this.m_vendorUserData) {
        vendorPanelData = this.m_vendorUserData.vendorData;
        vendorData = vendorPanelData.data;
        this.m_screen = CyberwareScreenType.Ripperdoc;
        this.m_VendorDataManager = new VendorDataManager();
        this
            .m_VendorDataManager
            .Initialize(this.GetPlayerControlledObject(), vendorData.entityID);
    } else {
        this.m_screen = CyberwareScreenType.Inventory;
    }
    this.m_statsSystem = GameInstance.GetStatsSystem(this.m_player.GetGame());
    this.m_statsDataSystem = GameInstance.GetStatsDataSystem(this.m_player.GetGame());
    this.m_statusEffectSystem = GameInstance.GetStatusEffectSystem(this.m_player.GetGame());
    this.m_inventorySystem = GameInstance.GetInventoryManager(this.m_player.GetGame());
    this.RegisterInventoryListener(this.GetPlayerControlledObject());
    this.PopulateCategories();
    this.SpawnMinigrids();
    this.Init();
    GameInstance.GetTelemetrySystem(this.GetPlayerControlledObject().GetGame()).LogVendorMenuState(this.m_VendorDataManager.GetVendorID(), true);
    this.m_filterArea = gamedataEquipmentArea.Invalid;
    this.m_hoverArea = gamedataEquipmentArea.Invalid;
    this.m_dollHoverArea = gamedataEquipmentArea.Invalid;
    this.m_ripperdocHoverState = RipperdocHoverState.None;
    this.m_defaultTooltipsMargin = new inkMargin(60.0, 60.0, 0.0, 0.0);
    super.OnInitialize();
    this.m_hasEquipEventTriggered = true;
    this.m_hasUnequipEventTriggered = true;
    i = 0;

    while i < ArraySize(this.m_categories) {
        ArrayPush(this.m_allFilters, this.m_categories[i].equipArea);
        ArrayPush(names, this.GetAreaHeader(this.m_allFilters[i]));
        ArrayPush(this.m_cachedAvailableItemsCounters, 0);
        ArrayPush(this.m_cachedVendorItemsCounters, 0);
        ArrayPush(this.m_cachedPlayerItemsCounters, 0);
        i += 1;
    }
    this.m_selector.Configure(names);
    this.m_minigridTargetAnchorMargin = inkWidgetRef.GetMargin(this.m_minigridTargetAnchor);
    this.m_minigridSelectorLeftAnchorMargin = inkWidgetRef.GetMargin(this.m_minigridSelectorLeftAnchor);
    this.m_minigridSelectorRightAnchorMargin = inkWidgetRef.GetMargin(this.m_minigridSelectorRightAnchor);
    this.m_developmentDataManager = new PlayerDevelopmentDataManager();
    this.m_developmentDataManager.Initialize(
            GameInstance
                .GetPlayerSystem(this.GetPlayerControlledObject().GetGame())
                .GetLocalPlayerMainGameObject() as PlayerPuppet,
            this
        );
    this.DisplayInventory(false);
    this.m_maxCapacityPossible = this.GetMaxCapacityPossible();
    this.SpawnPerks();
    if NotEquals(this.m_screen, CyberwareScreenType.Inventory) {
        inkWidgetRef.SetVisible(this.m_upgradeResourcesAnchor, true);
        this.PopulateCraftingMaterials();
    } else {
        inkWidgetRef.SetVisible(this.m_upgradeResourcesAnchor, false);
    }
    this.RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnReleaseInput");
    this.InitFacePaperdoll();
    this.m_isReturningPlayer = Cast<Bool>(this.m_questSystem.GetFact(n"ripperdoc_screen_glitched")) && Equals(this.m_screen, CyberwareScreenType.Inventory);
    this.m_vikTutorial = this.m_VendorDataManager.GetVendorID() == t"Vendors.wat_lch_ripperdoc_01" && this.m_questSystem.GetFact(n"q001_ripperdoc_done") == 0 && Equals(this.m_screen, CyberwareScreenType.Ripperdoc);
    this.m_isTutorial = this.m_vikTutorial;
    this.m_mq048TutorialFact = this.m_questSystem.GetFact(n"mq048_active") > 0 && this.m_questSystem.GetFact(n"mq048_done") == 0 && Equals(this.m_screen, CyberwareScreenType.Ripperdoc);
    this.m_isTutorial = (this.m_isTutorial || this.m_mq048TutorialFact) && this.m_questSystem.GetFact(n"ep1_ripperdoc_tutorial_seen") == 0;
    this.m_ep1StandaloneTutorial = this.m_questSystem.GetFact(n"ep1_standalone") > 0
        && this.m_questSystem.GetFact(n"ep1_ripperdoc_tutorial_seen") == 0
        && Equals(this.m_screen, CyberwareScreenType.Ripperdoc);
    if !this.m_isTutorial && this.m_ep1StandaloneTutorial {
        this.m_isTutorial = true;
        if this.m_questSystem.GetFact(n"ep1_ripperdoc_tutorial_started") < 1 {
            this.m_questSystem.SetFact(n"tutorial_ripperdoc_eyes_passed", 0);
            this.m_questSystem.SetFact(n"tutorial_ripperdoc_hands_passed", 0);
            this.m_questSystem.SetFact(n"tutorial_ripperdoc_armor_passed", 0);
            this.m_questSystem.SetFact(n"ep1_ripperdoc_tutorial_started", 1);
        }
    }

    if this.m_isTutorial && this.m_questSystem.GetFact(n"ngplus_active") > 0 {
        LogChannel(n"DEBUG", "RipperDocGameController::OnInitialize, NG+ detected, unsetting tutorial and resolving status effect...");
        this.m_vikTutorial = false;
        this.m_isTutorial = false;
        this.m_mq048TutorialFact = false;
        this.m_ep1StandaloneTutorial = false;
        this.AddTutorialItemsToVendorNgPlus();
    }

    if this.m_isTutorial {
        this
            .m_statusEffectSystem
            .ApplyStatusEffect(
                this.m_player.GetEntityID(),
                t"BaseStatusEffect.CyberwareTutorialAdjustments"
            );
        tutorialItemQuality = RPGManager
            .ConvertPlayerLevelToCyberwareQuality(
                GameInstance
                    .GetStatsSystem(this.m_player.GetGame())
                    .GetStatValue(
                        Cast<StatsObjectID>(this.m_playerID),
                        gamedataStatType.Level
                    ),
                false
            );
        requiredCapacity = this
            .m_statsSystem
            .GetStatValue(
                Cast<StatsObjectID>(this.m_playerID),
                gamedataStatType.HumanityOverallocated
            )
            - this
                .m_statsSystem
                .GetStatValue(
                    Cast<StatsObjectID>(this.m_playerID),
                    gamedataStatType.HumanityAvailable
                );
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_eyes_passed") < 1 {
            this.m_tutorialEyesCW = RipperDocGameController.GetAppropriateEyesTutorialCyberware(tutorialItemQuality);
            freedCapacity += this.UnequipAllFromGrid(gamedataEquipmentArea.EyesCW);
        }
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_hands_passed") < 1 {
            this.m_tutorialHandsCW = this.GetAppropriateHandsTutorialCyberware(tutorialItemQuality);
            freedCapacity += this.UnequipAllFromGrid(gamedataEquipmentArea.HandsCW);
        }
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_armor_passed") < 1 {
            this.m_tutorialArmorCW = RipperDocGameController.GetAppropriateArmorTutorialCyberware(tutorialItemQuality);
            freedCapacity
                += this
                    .UnequipAllFromGrid(gamedataEquipmentArea.IntegumentarySystemCW);
        }
        requiredCapacity -= freedCapacity;
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_eyes_passed") < 1 {
            requiredCapacity
                += RipperDocGameController
                    .GetTutorialItemCapacityRequirement(this.m_tutorialEyesCW, this.m_player);
        }
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_hands_passed") < 1 {
            requiredCapacity
                += RipperDocGameController
                    .GetTutorialItemCapacityRequirement(this.m_tutorialHandsCW, this.m_player);
        }
        if this.m_questSystem.GetFact(n"tutorial_ripperdoc_armor_passed") < 1 {
            requiredCapacity
                += RipperDocGameController
                    .GetTutorialItemCapacityRequirement(this.m_tutorialArmorCW, this.m_player);
        }
        this.AddTutorialItemsToStock(gamedataEquipmentArea.Invalid);
        if requiredCapacity > 0.0 {
            freedCapacity += requiredCapacity;
            freedCapacity -= this.FreeUpTheCapacityForTutorial(requiredCapacity);
        }
        this
            .AsyncSpawnFromLocal(
                inkWidgetRef.Get(this.m_capacityTutorialAnchor),
                n"tutorial_popup_capacity",
                this
            );
        this
            .AsyncSpawnFromLocal(
                inkWidgetRef.Get(this.m_armorTutorialAnchor),
                n"tutorial_popup_armor",
                this
            );
        this
            .AsyncSpawnFromLocal(
                inkWidgetRef.Get(this.m_slotsTutorialAnchor),
                n"tutorial_slots",
                this
            );
        currentAllocatedCapacity = this
            .m_statsSystem
            .GetStatValue(
                Cast<StatsObjectID>(this.m_playerID),
                gamedataStatType.HumanityAllocated
            ) - freedCapacity;
        this.m_tutorialZeroCapacityModifier = RPGManager
            .CreateStatModifier(
                gamedataStatType.HumanityAllocated,
                gameStatModifierType.Additive,
                -currentAllocatedCapacity
            );
        this
            .m_statsSystem
            .AddModifier(
                Cast<StatsObjectID>(this.m_player.GetEntityID()),
                this.m_tutorialZeroCapacityModifier
            );
    }
    this.m_selector.SetIsInTutorial(this.m_isTutorial);
    inkWidgetRef.SetVisible(this.m_selectorAnchor, !this.m_isTutorial);
    this.ShowMainScreenTutorials();
    this.m_audioSystem.Play(n"ui_gui_cyberware_tab_open");
    if MarketSystem.IsAttached(this.m_VendorDataManager.GetVendorInstance()) {
        this.OnUIVendorAttachedEvent(null);
    }
    if Equals(this.m_screen, CyberwareScreenType.Inventory) {
        this.PreparePlayerItems();
    }
    this.SetButtonHints(true, true);
}
