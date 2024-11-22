module NGPlus.InitialStatsFix

@addField(CharacterCreationGenderSelectionMenu)
let m_ngPlusSaveData: ref<NGPlusProgressionData>;

@addMethod(CharacterCreationGenderSelectionMenu)
private final func SetNGPlusAttributePreset() -> Void {
    let playerDevelopmentResults = this.m_ngPlusSaveData.GetPlayerDevelopmentSystemResults();
    let statsSystemResults = this.m_ngPlusSaveData.GetStatsSystemResults();

    this
        .m_characterCustomizationState
        .SetAttributePointsAvailable(Cast<Uint32>(playerDevelopmentResults.GetAttributePoints()));
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Strength,
            Cast<Uint32>(statsSystemResults.GetBody())
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Reflexes,
            Cast<Uint32>(statsSystemResults.GetReflexes())
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.TechnicalAbility,
            Cast<Uint32>(statsSystemResults.GetTechnicalAbility())
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Intelligence,
            Cast<Uint32>(statsSystemResults.GetIntelligence())
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Cool,
            Cast<Uint32>(statsSystemResults.GetCool())
        );
}

@replaceMethod(CharacterCreationGenderSelectionMenu)
protected cb func OnInitialize() -> Bool {
    let ngPlusSystem = GameInstance.GetNewGamePlusSystem();

    let ngPlusQuest = ngPlusSystem.GetNewGamePlusQuest();

    let isNgPlusActive = Equals(ngPlusQuest, ENGPlusType.StartFromQ001) || Equals(ngPlusQuest, ENGPlusType.StartFromQ101);

    let isStandalone = Equals(ngPlusQuest, ENGPlusType.StartFromQ101_ProgressionBuild);

    super.OnInitialize();
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_male, n"OnRelease", this, n"OnReleaseMale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_female, n"OnRelease", this, n"OnReleaseFemale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_male, n"OnPress", this, n"OnPressMale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_female, n"OnPress", this, n"OnPressFemale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_male, n"OnHoverOver", this, n"OnHoverOverMale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_female, n"OnHoverOver", this, n"OnHoverOverFemale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_male, n"OnHoverOut", this, n"OnHoverOutMale");
    inkWidgetRef
        .RegisterToCallback(this.m_streetRat_female, n"OnHoverOut", this, n"OnHoverOutFemale");
    if this.m_characterCustomizationState.IsExpansionStandalone() || isStandalone {
        this.SetEP1AttributePreset();
    } else {
        if isNgPlusActive {
            this.m_ngPlusSaveData = ngPlusSystem.GetProgressionData();
            this.SetNGPlusAttributePreset();
        } else {
            this.SetAttributePreset(this.m_characterCustomizationState.GetLifePath());
        }
    }
    this
        .GetTelemetrySystem()
        .LogInitialChoiceSetStatege(telemetryInitalChoiceStage.Gender);
    this.OnIntro();
}

@addField(CharacterCreationStatsMenu)
let m_isNgPlusActive: Bool;

@addField(CharacterCreationStatsMenu)
let m_ngPlusSaveData: ref<NGPlusProgressionData>;

@wrapMethod(CharacterCreationStatsMenu)
protected cb func OnInitialize() -> Bool {
    let ngPlusSystem = GameInstance.GetNewGamePlusSystem();
    let ngPlusQuest = ngPlusSystem.GetNewGamePlusQuest();
    this.m_isNgPlusActive = Equals(ngPlusQuest, ENGPlusType.StartFromQ001) || Equals(ngPlusQuest, ENGPlusType.StartFromQ101);
    this.m_ngPlusSaveData = ngPlusSystem.GetProgressionData();

    return wrappedMethod();
}

@replaceMethod(CharacterCreationStatsMenu)
private final func CanBeIncremented(currValue: Int32) -> Bool {
    if this.m_isNgPlusActive {
        return this.m_attributePointsAvailable > 0 && currValue < 20;
    }
    let maxLimit: Int32 = TweakDBInterface.GetInt(t"UICharacterCreationGeneral.BaseValues.maxAttributeValue", 0);

    return currValue < maxLimit && this.m_attributePointsAvailable > 0;
}

@replaceMethod(CharacterCreationStatsMenu)
private final func ReachedLimit(currValue: Int32) -> Bool {
    if this.m_isNgPlusActive {
        return currValue < 20;
    }

    let maxLimit: Int32 = TweakDBInterface.GetInt(t"UICharacterCreationGeneral.BaseValues.maxAttributeValue", 0);
    return currValue < maxLimit;
}

@replaceMethod(CharacterCreationStatsMenu)
private final func ResetAllBtnBackToBaseline() -> Void {
    let i: Int32 = 0;

    while i < ArraySize(this.m_attributesControllers) {
        this.m_attributesControllers[i].data.SetValue(3);
        this.m_attributesControllers[i].Refresh();
        i += 1;
    }
    if this.m_isNgPlusActive {
        let minAttributeValue = 3;
        let statsSystemResults = this.m_ngPlusSaveData.GetStatsSystemResults();

        let bodyAttributeAdded: Int32 = Cast<Int32>(statsSystemResults.GetBody()) - minAttributeValue;
        let reflexAttributeAdded: Int32 = Cast<Int32>(statsSystemResults.GetReflexes()) - minAttributeValue;
        let techAttributeAdded: Int32 = Cast<Int32>(statsSystemResults.GetTechnicalAbility()) - minAttributeValue;
        let intAttributeAdded: Int32 = Cast<Int32>(statsSystemResults.GetIntelligence()) - minAttributeValue;
        let coolAttributeAdded: Int32 = Cast<Int32>(statsSystemResults.GetCool()) - minAttributeValue;

        this.m_startingAttributePoints = bodyAttributeAdded
            + reflexAttributeAdded
            + techAttributeAdded
            + intAttributeAdded
            + coolAttributeAdded;
    } else {
        this.m_startingAttributePoints = 7;
    }

    this.m_attributePointsAvailable = this.m_startingAttributePoints;
    inkTextRef.SetText(this.m_skillPointLabel, ToString(this.m_attributePointsAvailable));
    this.RefreshPointsLabel();
    this.ManageAllButtonsVisibility();
}

// NOTE: fixes for standalone Q101...
@replaceMethod(CharacterCreationStatsMenu)
protected cb func OnInitialize() -> Bool {
    let attribute01Controller: ref<characterCreationStatsAttributeBtn>;
    let attribute02Controller: ref<characterCreationStatsAttributeBtn>;
    let attribute03Controller: ref<characterCreationStatsAttributeBtn>;
    let attribute04Controller: ref<characterCreationStatsAttributeBtn>;
    let attribute05Controller: ref<characterCreationStatsAttributeBtn>;
    let attributeType: gamedataStatType;
    super.OnInitialize();
    this.RequestCameraChange(n"Summary_Preview");
    if this.m_characterCustomizationState.IsExpansionStandalone() || Equals(GameInstance.GetNewGamePlusSystem().GetNewGamePlusQuest(), ENGPlusType.StartFromQ101_ProgressionBuild) {
        this.SkipStatsMenu();
    }
    attributeType = gamedataStatType.Strength;
    attribute01Controller = inkWidgetRef.GetController(this.m_attribute_01) as characterCreationStatsAttributeBtn;
    attribute01Controller
        .SetData(
            attributeType,
            Cast<Int32>(this.m_characterCustomizationState.GetAttribute(attributeType))
        );
    attributeType = gamedataStatType.Intelligence;
    attribute02Controller = inkWidgetRef.GetController(this.m_attribute_02) as characterCreationStatsAttributeBtn;
    attribute02Controller
        .SetData(
            attributeType,
            Cast<Int32>(this.m_characterCustomizationState.GetAttribute(attributeType))
        );
    attributeType = gamedataStatType.Reflexes;
    attribute03Controller = inkWidgetRef.GetController(this.m_attribute_03) as characterCreationStatsAttributeBtn;
    attribute03Controller
        .SetData(
            attributeType,
            Cast<Int32>(this.m_characterCustomizationState.GetAttribute(attributeType))
        );
    attributeType = gamedataStatType.TechnicalAbility;
    attribute04Controller = inkWidgetRef.GetController(this.m_attribute_04) as characterCreationStatsAttributeBtn;
    attribute04Controller
        .SetData(
            attributeType,
            Cast<Int32>(this.m_characterCustomizationState.GetAttribute(attributeType))
        );
    attributeType = gamedataStatType.Cool;
    attribute05Controller = inkWidgetRef.GetController(this.m_attribute_05) as characterCreationStatsAttributeBtn;
    attribute05Controller
        .SetData(
            attributeType,
            Cast<Int32>(this.m_characterCustomizationState.GetAttribute(attributeType))
        );
    ArrayClear(this.m_attributesControllers);
    ArrayPush(this.m_attributesControllers, attribute01Controller);
    ArrayPush(this.m_attributesControllers, attribute02Controller);
    ArrayPush(this.m_attributesControllers, attribute03Controller);
    ArrayPush(this.m_attributesControllers, attribute04Controller);
    ArrayPush(this.m_attributesControllers, attribute05Controller);
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_01, n"OnValueIncremented", this, n"OnValueIncremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_01, n"OnValueDecremented", this, n"OnValueDecremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_02, n"OnValueIncremented", this, n"OnValueIncremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_02, n"OnValueDecremented", this, n"OnValueDecremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_03, n"OnValueIncremented", this, n"OnValueIncremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_03, n"OnValueDecremented", this, n"OnValueDecremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_04, n"OnValueIncremented", this, n"OnValueIncremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_04, n"OnValueDecremented", this, n"OnValueDecremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_05, n"OnValueIncremented", this, n"OnValueIncremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_05, n"OnValueDecremented", this, n"OnValueDecremented");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_01, n"OnBtnHoverOver", this, n"OnBtnHoverOver");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_02, n"OnBtnHoverOver", this, n"OnBtnHoverOver");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_03, n"OnBtnHoverOver", this, n"OnBtnHoverOver");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_04, n"OnBtnHoverOver", this, n"OnBtnHoverOver");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_05, n"OnBtnHoverOver", this, n"OnBtnHoverOver");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_01, n"OnBtnHoverOut", this, n"OnBtnHoverOut");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_02, n"OnBtnHoverOut", this, n"OnBtnHoverOut");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_03, n"OnBtnHoverOut", this, n"OnBtnHoverOut");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_04, n"OnBtnHoverOut", this, n"OnBtnHoverOut");
    inkWidgetRef
        .RegisterToCallback(this.m_attribute_05, n"OnBtnHoverOut", this, n"OnBtnHoverOut");
    this
        .RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnShortcutPress");
    this.RegisterToCallback(n"OnEnter", this, n"OnHoverOverWidget");
    this.RegisterToCallback(n"OnLeave", this, n"OnHoverOutWidget");
    inkWidgetRef
        .RegisterToCallback(this.m_previousPageBtn, n"OnRelease", this, n"OnPreviousButton");
    inkWidgetRef
        .RegisterToCallback(
            this.m_ConfirmationConfirmBtn,
            n"OnRelease",
            this,
            n"OnConfirmationConfirm"
        );
    inkWidgetRef
        .RegisterToCallback(this.m_ConfirmationCloseBtn, n"OnRelease", this, n"OnConfirmationClose");
    this.RefreshControllers();
    this.PrepareTooltips();
    this.SetDefaultTooltip();
    this.m_toolTipOffset.left = 60.0;
    this.m_toolTipOffset.top = 5.0;
    this.m_attributePointsAvailable = Cast<Int32>(this.m_characterCustomizationState.GetAttributePointsAvailable());
    inkTextRef.SetText(this.m_skillPointLabel, ToString(this.m_attributePointsAvailable));
    this.RefreshPointsLabel();
    this.ManageAllButtonsVisibility();
    this
        .GetTelemetrySystem()
        .LogInitialChoiceSetStatege(telemetryInitalChoiceStage.Attributes);
    inkWidgetRef.SetVisible(this.m_optionSwitchHint, false);
    this.OnIntro();
}

@replaceMethod(characterCreationSummaryMenu)
protected cb func OnInitialize() -> Bool {
    let isStandalone = this.m_characterCustomizationState.IsExpansionStandalone() || Equals(GameInstance.GetNewGamePlusSystem().GetNewGamePlusQuest(), ENGPlusType.StartFromQ101_ProgressionBuild);

    super.OnInitialize();
    this.SetUpLifePath();
    this.SetUpAttribiutes();
    this.SetUpDifficulty();
    inkWidgetRef.RegisterToCallback(this.m_previousPageBtn, n"OnRelease", this, n"OnPreviousButton");
    inkWidgetRef.RegisterToCallback(this.m_glitchBtn, n"OnRelease", this, n"OnGlitchButton");
    inkWidgetRef.SetVisible(this.m_unsetAttributeWrapper, !isStandalone);
    inkWidgetRef.SetVisible(this.m_expansionInfoWrapper, isStandalone);
    this.m_glitchClicks = 0;
    this.m_loadingFinished = false;
    this.OnIntro();
}