// WIP
// Not ready yet

enum NewGamePlusStatsAdjustmentActiveScreens {
    Categories = 0,
    Perks = 1,
    Espionage = 2,
    Count = 3,
    Invalid = -1
}

public class NewGamePlusStatsAdjustmentController extends BaseCharacterCreationController {
    // WOLVENKIT INITIALIZED

    // Root->leftTooltipFolder
    private let m_attributeTooltipHolderLeft: inkWidgetRef;

    // Root->rightTooltipFolder
    private let m_attributeTooltipHolderRight: inkWidgetRef;

    // Root->button_hints
    private let m_buttonHintsManagerRef: inkWidgetRef;

    // Root->attributes_canvas->center->bg->left->inner_frame_hover
    private let m_centerHighlightLeftInnerHover: inkWidgetRef;
    // Root->attributes_canvas->center->bg->right->inner_frame_hover
    private let m_centerHighlightRightInnerHover: inkWidgetRef;
    // Root->attributes_canvas->center->bg->left->frame_hover
    private let m_centerHighlightLeftHover: inkWidgetRef;
    // Root->attributes_canvas->center->bg->right->frame_hover
    private let m_centerHighlightRightHover: inkWidgetRef;

    // Root->attributes_canvas->available_points->list->valueWrapper3
    private let m_espionagePointsRef: inkWidgetRef;
    
    // Root->espionageScreenWrapper
    private let m_espionageScreenContainer: inkWidgetRef;

    // Root->attributes_canvas
    private let m_perksCategoriesContainer: inkWidgetRef;

    // Root->perksScreenWrapper
    private let m_perksScreenContainer: inkWidgetRef;

    // Root->left_perk_tooltip_bg
    private let m_perkTooltipBgLeft: inkWidgetRef;
    // Root->right_perk_tooltip_bg
    private let m_perkTooltipBgRight: inkWidgetRef;
    // Root->left_perks_tooltip_folder
    private let m_perkTooltipPlacementLeft: inkWidgetRef;
    // Root->right_perks_tooltip_folder
    private let m_perkTooltipPlacementRight: inkWidgetRef;
    // Root->attributes_canvas->center->bg->level_value
    private let m_playerLevel: inkTextRef;
    // Root->attributes_canvas->available_points
    private let m_pointsDisplay: inkWidgetRef;
    // Root->attributes_canvas->attributeResetButton
    private let m_resetAttributesButton: inkWidgetRef;
    // Root->attributes_canvas->skillScreenButton
    private let m_skillsScreenButton: inkWidgetRef;

    // Root->tabsContainer
    private let m_tabsContainer: inkWidgetRef;

    // Root
    private let m_tooltipsManagerRef: inkWidgetRef;
    // WOLVENKIT INITIALIZED END

    private let m_espionageAttributeMask: inkWidgetRef;
    private let m_centerHighlightParts: array<inkWidgetRef>;
    private let m_perkTooltipBgAnimProxy: ref<inkAnimProxy>;
    private let m_menuEventDispatcher: wref<inkMenuEventDispatcher>;
    private let m_tabsController: wref<NewPerkTabsController>;
    private let m_perksScreenController: wref<NewGamePlusPerksScreenLogicController>;
    private let m_espionageScreenController: wref<NewGamePlusPerksScreenLogicController>;
    private let m_tooltipsManager: wref<gameuiTooltipsManager>;
    private let m_buttonHintsController: wref<ButtonHints>;
    private let m_dataManager: ref<PlayerDevelopmentDataManager>;
    private let m_questSystem: wref<QuestsSystem>;
    private let m_attributesControllersList: array<wref<PerksMenuAttributeItemController>>;
    private let m_perksMenuItemCreatedQueue: array<ref<PerksMenuAttributeItemCreated>>;
    private let m_pointsDisplayController: wref<PerksPointsDisplayController>;
    private let m_playerStatsBlackboard: wref<IBlackboard>;
    private let m_characterLevelListener: ref<CallbackHandle>;
    private let m_player: wref<PlayerPuppet>;
    private let m_previousScreen: NewGamePlusStatsAdjustmentActiveScreens;
    private let m_currentScreen: NewGamePlusStatsAdjustmentActiveScreens;
    private let m_currentStatScreen: gamedataStatType;
    private let m_lastHoveredAttribute: PerkMenuAttribute;
    private let m_cyberwarePerkDetailsPopupToken: ref<inkGameNotificationToken>;
    private let m_perksScreenIntroAnimProxy: ref<inkAnimProxy>;
    private let m_perksScreenOutroAnimProxy: ref<inkAnimProxy>;
    private let m_perksScreenDirection: NewPerkTabsArrowDirection;
    private let m_currentTooltipData: PerkHoverEventTooltipData;
    private let m_uiSystem: ref<UISystem>;
    private let m_currentCursorPos: Vector2;
    private let m_perkUserData: ref<PerkUserData>;
    private let m_vendorUserData: ref<VendorUserData>;
    private let m_skillsOpenData: ref<OpenSkillsMenuData>;
    private let m_resetConfirmationToken: ref<inkGameNotificationToken>;
    private let m_userData: ref<IScriptable>;
    private let m_screenDisplayContext: ScreenDisplayContext;

    private final func InitializeNonInkVariables() {
        this.m_perksScreenDirection = NewPerkTabsArrowDirection.Left;
        this.m_lastHoveredAttribute = PerkMenuAttribute.Body;
        this.m_currentScreen = NewGamePlusStatsAdjustmentActiveScreens.Categories;
        this.m_previousScreen = NewGamePlusStatsAdjustmentActiveScreens.Invalid;
        this.m_screenDisplayContext = ScreenDisplayContext.Default;
        this.m_currentStatScreen = gamedataStatType.Invalid;

        ArrayPush(this.m_centerHighlightParts, this.m_centerHighlightLeftHover);
        ArrayPush(this.m_centerHighlightParts, this.m_centerHighlightRightHover);
        ArrayPush(this.m_centerHighlightParts, this.m_centerHighlightLeftInnerHover);
        ArrayPush(this.m_centerHighlightParts, this.m_centerHighlightRightInnerHover);
    }

    protected cb func OnInitialize() -> Bool {
        this.InitializeNonInkVariables();

        //this.AsyncSpawnFromLocal(inkWidgetRef.Get(this.m_tabsContainer), n"Tabs", this, n"OnTabsSpawned");
        this.AsyncSpawnFromLocal(inkWidgetRef.Get(this.m_perksScreenContainer), n"PerksScreen", this, n"OnPerksScreenSpawned");
        this.AsyncSpawnFromLocal(inkWidgetRef.Get(this.m_espionageScreenContainer), n"Spy_PerksScreen", this, n"OnEspionageScreenSpawned");
        this.m_pointsDisplayController = inkWidgetRef.GetController(this.m_pointsDisplay) as PerksPointsDisplayController;
        
        this.m_perksScreenDirection = NewPerkTabsArrowDirection.Invalid;
        this.UpdateScreen();
        
        this.m_buttonHintsController = this.SpawnFromExternal(inkWidgetRef.Get(this.m_buttonHintsManagerRef), r"base\\gameplay\\gui\\common\\buttonhints.inkwidget", n"Root").GetController() as ButtonHints;
        this.m_buttonHintsController.AddButtonHint(n"back", GetLocalizedText("Common-Access-Close"));
        this.m_tooltipsManager = inkWidgetRef.GetControllerByType(this.m_tooltipsManagerRef, n"gameuiTooltipsManager") as gameuiTooltipsManager;
        this.m_tooltipsManager.Setup(ETooltipsStyle.Menus);
        inkWidgetRef.RegisterToCallback(this.m_resetAttributesButton, n"OnRelease", this, n"OnResetAttributesButtonClick");
        inkWidgetRef.RegisterToCallback(this.m_resetAttributesButton, n"OnHoverOver", this, n"OnResetAttributesButtonHoverOver");
        inkWidgetRef.RegisterToCallback(this.m_resetAttributesButton, n"OnHoverOut", this, n"OnResetAttributesButtonHoverOut");
        //inkWidgetRef.RegisterToCallback(this.m_skillsScreenButton, n"OnRelease", this, n"OnSkillScreenButtonClick");
        //inkWidgetRef.RegisterToCallback(this.m_skillsScreenButton, n"OnHoverOver", this, n"OnSkillScreenButtonHoverOver");
        //inkWidgetRef.RegisterToCallback(this.m_skillsScreenButton, n"OnHoverOut", this, n"OnSkillScreenButtonHoverOut");
        this.RegisterToGlobalInputCallback(n"OnPostOnRelative", this, n"OnRelativeInput");
        this.RegisterToGlobalInputCallback(n"OnPostOnAxis", this, n"OnAxisInput");
        this.HandleEventQueue();
    }

    protected cb func OnUninitialize() -> Bool {
        this.m_menuEventDispatcher.UnregisterFromEvent(n"OnBack", this, n"OnBack");
        this.m_menuEventDispatcher.UnregisterFromEvent(n"OnBeforeLeaveScenario", this, n"OnBeforeLeaveScenario");
        this.m_menuEventDispatcher.UnregisterFromEvent(n"OnSetScreenDisplayContext", this, n"OnSetScreenDisplayContext");
        inkWidgetRef.UnregisterFromCallback(this.m_resetAttributesButton, n"OnRelease", this, n"OnResetAttributesButtonClick");
        inkWidgetRef.UnregisterFromCallback(this.m_resetAttributesButton, n"OnHoverOver", this, n"OnResetAttributesButtonHoverOver");
        inkWidgetRef.UnregisterFromCallback(this.m_resetAttributesButton, n"OnHoverOut", this, n"OnResetAttributesButtonHoverOut");
        //inkWidgetRef.UnregisterFromCallback(this.m_skillsScreenButton, n"OnRelease", this, n"OnSkillScreenButtonClick");
        //inkWidgetRef.UnregisterFromCallback(this.m_skillsScreenButton, n"OnHoverOver", this, n"OnSkillScreenButtonHoverOver");
        //inkWidgetRef.UnregisterFromCallback(this.m_skillsScreenButton, n"OnHoverOut", this, n"OnSkillScreenButtonHoverOut");
        this.UnregisterFromGlobalInputCallback(n"OnPostOnRelative", this, n"OnRelativeInput");
        this.UnregisterFromGlobalInputCallback(n"OnPostOnAxis", this, n"OnAxisInput");
        this.StopRumbleLoop(RumbleStrength.SuperLight);
    }

    protected cb func OnSetScreenDisplayContext(userData: ref<IScriptable>) -> Bool {
        let displayContext: ref<ScreenDisplayContextData> = userData as ScreenDisplayContextData;
        if IsDefined(displayContext) {
            this.m_screenDisplayContext = displayContext.Context;
        };
    }

    private final func StopPerkScreenAnims() -> Void {
        if IsDefined(this.m_perksScreenIntroAnimProxy) && this.m_perksScreenIntroAnimProxy.IsPlaying() {
        this.m_perksScreenIntroAnimProxy.UnregisterFromAllCallbacks(inkanimEventType.OnFinish);
        this.m_perksScreenIntroAnimProxy.GotoEndAndStop();
        };
        if IsDefined(this.m_perksScreenOutroAnimProxy) && this.m_perksScreenOutroAnimProxy.IsPlaying() {
        this.m_perksScreenOutroAnimProxy.UnregisterFromAllCallbacks(inkanimEventType.OnFinish);
        this.m_perksScreenOutroAnimProxy.GotoStartAndStop();
        };
    }

    private final func PlayScreenIntro() -> ref<inkAnimProxy> {
        let animName: CName;
        this.StopPerkScreenAnims();
        if Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Invalid) {
            animName = Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage) ? n"panel_perks_espionage_intro" : n"panel_perks_intro";
        } else {
        if Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage) {
            animName = Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Left) ? n"swipe_right_2_espionage_screen" : n"swipe_left_2_espionage_screen";
        } else {
            animName = Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Left) ? n"swipe_right_2_perks_screen" : n"swipe_left_2_perks_screen";
        };
        };
        switch this.m_currentScreen {
        case NewGamePlusStatsAdjustmentActiveScreens.Categories:
            return this.PlayLibraryAnimation(n"panel_categories_intro");
        case NewGamePlusStatsAdjustmentActiveScreens.Perks:
            return this.PlayLibraryAnimationOnAutoSelectedTargets(animName, this.m_perksScreenController.GetRootWidget());
        case NewGamePlusStatsAdjustmentActiveScreens.Espionage:
            return this.PlayLibraryAnimationOnAutoSelectedTargets(animName, this.m_espionageScreenController.GetRootWidget());
        default:
            return null;
        };
    }

    private final func PlayScreenOutro() -> ref<inkAnimProxy> {
        let animName: CName;
        this.StopPerkScreenAnims();
        if Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Invalid) {
        return null;
        };
        if Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage) {
        animName = Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Left) ? n"swipe_right_1_espionage_screen" : n"swipe_left_1_espionage_screen";
        } else {
        animName = Equals(this.m_perksScreenDirection, NewPerkTabsArrowDirection.Left) ? n"swipe_right_1_perks_screen" : n"swipe_left_1_perks_screen";
        };
        switch this.m_currentScreen {
        case NewGamePlusStatsAdjustmentActiveScreens.Perks:
            return this.PlayLibraryAnimationOnAutoSelectedTargets(animName, this.m_perksScreenController.GetRootWidget());
        case NewGamePlusStatsAdjustmentActiveScreens.Espionage:
            return this.PlayLibraryAnimationOnAutoSelectedTargets(animName, this.m_espionageScreenController.GetRootWidget());
        default:
            return null;
        };
    }

    private final func UpdateScreensVisibility() -> Void {
        inkWidgetRef.SetVisible(this.m_perksCategoriesContainer, Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Categories));
        inkWidgetRef.SetVisible(this.m_tabsContainer, Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Perks) || Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage));
        inkWidgetRef.SetVisible(this.m_perksScreenContainer, Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Perks));
        inkWidgetRef.SetVisible(this.m_espionageScreenContainer, Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage));
        this.m_perksScreenController.SetActive(Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Perks));
        this.m_espionageScreenController.SetActive(Equals(this.m_currentScreen, NewGamePlusStatsAdjustmentActiveScreens.Espionage));
    }

    protected cb func OnTabsSpawned(widget: ref<inkWidget>, userData: ref<IScriptable>) -> Bool {
        
    }

    protected cb func OnPerksScreenSpawned(widget: ref<inkWidget>, userData: ref<IScriptable>) -> Bool {
        this.m_perksScreenController = widget.GetController() as NewGamePlusPerksScreenLogicController;
    }

    protected cb func OnEspionageScreenSpawned(widget: ref<inkWidget>, userData: ref<IScriptable>) -> Bool {
        this.m_espionageScreenController = widget.GetController() as NewGamePlusPerksScreenLogicController;
    }

    private final func UpdateScreen() {

    }

    private final func HandleEventQueue() {

    }
}

public class NewGamePlusPerksScreenLogicController extends inkLogicController {
    private let m_rootWidget: inkWidgetRef;

    private let m_perksWidgets: array<inkWidgetRef>;
    private let m_gauge: inkWidgetRef;
    private let m_tiers: array<PerkScreenTierInfo>;
    private let m_animationBoldLineWidget: inkWidgetRef;
    private let m_animationLineWidget: inkWidgetRef;
    private let m_animationGradientWidget: inkWidgetRef;
    private let m_attributeButtonWidget: inkWidgetRef;
    private let m_lockedLineIcon: inkWidgetRef;
    private let m_unlockedLineIcon: inkWidgetRef;
    private let m_attributeRequirementTexts: array<inkTextRef>;
    private let m_levelRequirementTexts: array<inkTextRef>;

    private let m_perksInitialized: Bool;

    private let m_perksControllers: ref<inkHashMap>;

    private let m_perksContainersControllers: ref<inkHashMap>;

    private let m_perkControllersArray: array<wref<NewPerksPerkContainerLogicController>>;

    private let m_enabledControllers: array<wref<NewPerksPerkContainerLogicController>>;

    private let m_initData: ref<NewPerksScreenInitData>;

    private let m_perksList: array<wref<NewPerk_Record>>;

    private let m_attributePoints: Int32;

    private let m_linksManager: ref<NewPerksRequirementsLinksManager>;

    private let m_gaugeController: wref<NewPerksGaugeController>;

    private let m_attributeButtonController: wref<NewPerksAttributeButtonController>;

    private let m_buttonHintsController: wref<ButtonHints>;

    private let m_dataManager: wref<PlayerDevelopmentDataManager>;

    private let m_uiScriptableSystem: wref<UIScriptableSystem>;

    private let m_levels: array<NewPerksGaugePointDetails>;

    private let m_highlightData: array<PerkTierHighlight>;

    private let m_activeProxies: array<ref<inkAnimProxy>>;

    private let m_highlightedWires: array<inkWidgetRef>;

    private let m_highlightedPerks: array<wref<inkWidget>>;

    private let m_dimmedWidgets: array<inkWidgetRef>;

    private let m_dimProxies: array<ref<inkAnimProxy>>;

    private let m_undimProxies: array<ref<inkAnimProxy>>;

    private let m_isActiveScreen: Bool;

    private let m_isEspionage: Bool;

    private let m_unlockAnimData: UnlockAnimData;

    private let m_lineAnimProxy: ref<inkAnimProxy>;

    private let m_buttonMoveAnimProxy: ref<inkAnimProxy>;

    private let m_buttonCustomAnimProxy: ref<inkAnimProxy>;

    private let m_lockAnimProxy: ref<inkAnimProxy>;

    private let m_introFinished: Bool;

    private let m_perkHovered: Bool;

    private let m_currentHoveredPerkData: ref<NewPerkDisplayData>;

    private let m_gameController: wref<NewPerksCategoriesGameController>;

    private let m_sellFailToken: ref<inkGameNotificationToken>;

    private let m_perkToSnapCursor: gamedataNewPerkType;

    private let m_unlockState: Int32;

    private final func InitializePerkScreenTiers(rootWidget: ref<inkCanvas>) -> Void {
        let tierNovice = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_novice")));
        let tierAdept = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_adept")));
        let tierExpert = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_expert")));
        let tierMaster = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_master")));

        let highlightWidgetNovice = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Tier_Highlight", n"1")));
        let highlightWidgetAdept = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Tier_Highlight", n"2")));
        let highlightWidgetExpert = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Tier_Highlight", n"3")));
        let highlightWidgetMaster = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Tier_Highlight", n"4")));

        let attrLevelWrapperNovice = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_4")));
        let attrLevelWrapperAdept = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_9")));
        let attrLevelWrapperExpert = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_15")));
        let attrLevelWrapperMaster = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_20")));

        let requirementTextNovice = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_novice", n"tier_req")) as inkText);
        let requirementTextAdept = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_adept", n"tier_req")) as inkText);
        let requirementTextExpert = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_expert", n"tier_req")) as inkText);
        let requirementTextMaster = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_master", n"tier_req")) as inkText);

        let infoNovice = new PerkScreenTierInfo(
            tierNovice, 
            requirementTextNovice,
            attrLevelWrapperNovice,
            highlightWidgetNovice,
            false, 
            false
        );

        let infoAdept = new PerkScreenTierInfo(
            tierAdept, 
            requirementTextAdept,
            attrLevelWrapperAdept,
            highlightWidgetAdept,
            false, 
            false
        );

        let infoExpert = new PerkScreenTierInfo(
            tierExpert, 
            requirementTextExpert,
            attrLevelWrapperExpert,
            highlightWidgetExpert,
            false, 
            false
        );

        let infoMaster = new PerkScreenTierInfo(
            tierMaster, 
            requirementTextMaster,
            attrLevelWrapperMaster,
            highlightWidgetMaster,
            false, 
            false
        );

        ArrayPush(this.m_tiers, infoNovice);
        ArrayPush(this.m_tiers, infoAdept);
        ArrayPush(this.m_tiers, infoExpert);
        ArrayPush(this.m_tiers, infoMaster);
    }

    private final func InitializePerkContainers(rootWidget: ref<inkCanvas>) -> Void {
        let treeWidgetList: array<inkWidgetRef>;
        if this.m_isEspionage {
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"Inbetween_Left"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"Tree_Right"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"Tree_Left"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"Tree_Center"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"Inbetween_Right"))));
        } else {
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"tree_center"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"tree_left"))));
            ArrayPush(treeWidgetList, inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Trees", n"tree_right"))));
        }

        for widget in treeWidgetList {
            let canvas = inkWidgetRef.Get(widget) as inkCanvas;

            if IsDefined(canvas) {
                this.InitializePerkContainersInternal(canvas);
            }
        }
    }

    private final func InitializePerkContainersInternal(rootWidget: ref<inkCanvas>) -> Void {
        let treeChildren = rootWidget.GetNumChildren();

        let i = 0;
        while i < treeChildren {
            let treeChild = rootWidget.GetWidget(i) as inkCanvas;

            if IsDefined(treeChild) {
                let controller = treeChild.GetController() as NewPerksPerkContainerLogicController;

                if IsDefined(controller) {
                    let ref = inkWidgetRef.Create(treeChild);
                    ArrayPush(this.m_perksWidgets, ref);
                } else {
                    this.InitializePerkContainersInternal(treeChild);
                }
            }

            i += 1;
        }
    }
    
    private final func InitializeWidgetData() -> Void {
        let rootWidget = this.GetRootWidget() as inkCanvas;
        // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
        // FUCK INKWIDGETS
        if !this.m_isEspionage {
            this.m_animationBoldLineWidget = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"tier_background", n"tier_anim_01")));
            this.m_animationGradientWidget = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"tier_background", n"line_canvas", n"tier_gradient")));
            this.m_animationLineWidget = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"tier_background", n"line_canvas")));
            
            this.m_gauge = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"Gauge")));

            let tierReq4Text = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_4", n"attr_level", n"attribute")) as inkText);
            let tierReq9Text = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_9", n"attr_level", n"attribute")) as inkText);
            let tierReq15Text = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_15", n"attr_level", n"attribute")) as inkText);
            let tierReq20Text = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_20", n"attr_level", n"attribute")) as inkText);

            ArrayPush(this.m_attributeRequirementTexts, tierReq4Text);
            ArrayPush(this.m_attributeRequirementTexts, tierReq9Text);
            ArrayPush(this.m_attributeRequirementTexts, tierReq15Text);
            ArrayPush(this.m_attributeRequirementTexts, tierReq20Text);

            let tierReq4TextLevel = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_4", n"attr_level", n"tier_req")) as inkText);
            let tierReq9TextLevel = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_9", n"attr_level", n"tier_req")) as inkText);
            let tierReq15TextLevel = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_15", n"attr_level", n"tier_req")) as inkText);
            let tierReq20TextLevel = inkTextRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"TIERS_L_R", n"tier_req_20", n"attr_level", n"tier_req")) as inkText);

            ArrayPush(this.m_levelRequirementTexts, tierReq4TextLevel);
            ArrayPush(this.m_levelRequirementTexts, tierReq9TextLevel);
            ArrayPush(this.m_levelRequirementTexts, tierReq15TextLevel);
            ArrayPush(this.m_levelRequirementTexts, tierReq20TextLevel);

            this.m_lockedLineIcon = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"tier_background", n"line_canvas", n"line", n"ico_lock")));
            this.m_unlockedLineIcon = inkWidgetRef.Create(rootWidget.GetWidget(inkWidgetPath.Build(n"tier_background", n"line_canvas", n"line", n"ico_unlock_static_bg")));

            this.InitializePerkScreenTiers(rootWidget);
        }
        
        this.InitializePerkContainers(rootWidget);
    }

    public final func SetActive(state: Bool) {

    }
}

public class NewGamePlusPerkTabsController extends inkLogicController {
    private let m_tabText: inkTextRef;

    private let m_currentAttributePoints: inkTextRef;

    private let m_currentAttributeIcon: inkImageRef;

    private let m_leftArrow: inkWidgetRef;

    private let m_rightArrow: inkWidgetRef;

    private let m_attributePointsWrapper: inkWidgetRef;

    private let m_attributePointsText: inkTextRef;

    private let m_perkPointsWrapper: inkWidgetRef;

    private let m_perkPointsText: inkTextRef;

    private let m_espionagePointsWrapper: inkWidgetRef;

    private let m_espionagePointsText: inkTextRef;

    private let m_bars: array<inkWidgetRef>;

    private let m_dataManager: wref<PlayerDevelopmentDataManager>;

    private let m_initData: ref<NewPerksScreenInitData>;

    private let m_isEspionageUnlocked: Bool;

    public final func SetData(dataManager: wref<PlayerDevelopmentDataManager>, initData: wref<NewPerksScreenInitData>, opt isEspionageUnlocked: Bool) -> Void {
        this.m_dataManager = dataManager;
        this.m_initData = initData;
        this.m_isEspionageUnlocked = isEspionageUnlocked;
        let attributeData: ref<AttributeData> = dataManager.GetAttribute(dataManager.GetAttributeRecordIDFromEnum(initData.perkMenuAttribute));
        inkTextRef.SetText(this.m_tabText, attributeData.label);
        inkTextRef.SetText(this.m_currentAttributePoints, IntToString(attributeData.value));
        inkWidgetRef.SetVisible(this.m_currentAttributePoints, NotEquals(initData.perkMenuAttribute, PerkMenuAttribute.Espionage));
        inkImageRef.SetTexturePart(this.m_currentAttributeIcon, PerkAttributeHelper.GetIconAtlasPart(initData.perkMenuAttribute));
        inkWidgetRef.SetVisible(this.m_attributePointsWrapper, NotEquals(initData.perkMenuAttribute, PerkMenuAttribute.Espionage));
        inkWidgetRef.SetVisible(this.m_perkPointsWrapper, NotEquals(initData.perkMenuAttribute, PerkMenuAttribute.Espionage));
        inkWidgetRef.SetVisible(this.m_espionagePointsWrapper, Equals(initData.perkMenuAttribute, PerkMenuAttribute.Espionage));
        inkTextRef.SetText(this.m_attributePointsText, IntToString(this.m_dataManager.GetAttributePoints()));
        inkTextRef.SetText(this.m_perkPointsText, IntToString(this.m_dataManager.GetPerkPoints()));
        inkTextRef.SetText(this.m_espionagePointsText, IntToString(this.m_dataManager.GetSpyPerkPoints()));
        this.UpdateBars();
    }

    public final func SetValues(attributePointsVal: Int32, perkPointsVal: Int32, espionagePointsVal: Int32) -> Void {
        let attributeData: ref<AttributeData> = this.m_dataManager.GetAttribute(this.m_dataManager.GetAttributeRecordIDFromEnum(this.m_initData.perkMenuAttribute));
        inkTextRef.SetText(this.m_attributePointsText, IntToString(attributePointsVal));
        inkTextRef.SetText(this.m_perkPointsText, IntToString(perkPointsVal));
        inkTextRef.SetText(this.m_espionagePointsText, IntToString(espionagePointsVal));
        inkTextRef.SetText(this.m_currentAttributePoints, IntToString(attributeData.value));
    }

    private final func UpdateBars() -> Void {
        let attributeIndex: Int32;
        let i: Int32 = 0;
        let limit: Int32 = ArraySize(this.m_bars);
        while i < limit {
        attributeIndex = EnumInt(this.m_initData.perkMenuAttribute);
        inkWidgetRef.SetState(this.m_bars[i], i == attributeIndex ? n"Active" : n"Default");
        i += 1;
        };
        inkWidgetRef.SetVisible(this.m_bars[limit - 1], this.m_isEspionageUnlocked);
    }
}