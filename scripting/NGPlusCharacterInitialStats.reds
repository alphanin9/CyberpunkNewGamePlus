module NGPlus.InitialStatsFix

@addField(CharacterCreationGenderSelectionMenu)
let m_ngPlusSaveData: PlayerSaveData;

@addMethod(CharacterCreationGenderSelectionMenu)
private final func SetNGPlusAttributePreset() -> Void {
    this.m_characterCustomizationState.SetAttributePointsAvailable(Cast<Uint32>(this.m_ngPlusSaveData.playerAttributePoints));
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Strength,
            Cast<Uint32>(this.m_ngPlusSaveData.playerBodyAttribute)
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Reflexes,
            Cast<Uint32>(this.m_ngPlusSaveData.playerReflexAttribute)
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.TechnicalAbility,
            Cast<Uint32>(this.m_ngPlusSaveData.playerTechAttribute)
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Intelligence,
            Cast<Uint32>(this.m_ngPlusSaveData.playerIntelligenceAttribute)
        );
    this
        .m_characterCustomizationState
        .SetAttribute(
            gamedataStatType.Cool,
            Cast<Uint32>(this.m_ngPlusSaveData.playerCoolAttribute)
        );
}

@replaceMethod(CharacterCreationGenderSelectionMenu)
protected cb func OnInitialize() -> Bool {
    let isNgPlusActive = GameInstance.GetNewGamePlusSystem().GetNewGamePlusState();
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
    if this.m_characterCustomizationState.IsExpansionStandalone() {
        this.SetEP1AttributePreset();
    } else {
        if isNgPlusActive {
            this.m_ngPlusSaveData = GameInstance.GetNewGamePlusSystem().GetSaveData();
            this.SetNGPlusAttributePreset();
        } else {
            this
                .SetAttributePreset(this.m_characterCustomizationState.GetLifePath());
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
let m_ngPlusSaveData: PlayerSaveData;

@wrapMethod(CharacterCreationStatsMenu)
protected cb func OnInitialize() -> Bool {
    this.m_isNgPlusActive = GameInstance.GetNewGamePlusSystem().GetNewGamePlusState();
    this.m_ngPlusSaveData = GameInstance.GetNewGamePlusSystem().GetSaveData();

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
        let bodyAttributeAdded: Int32 = this.m_ngPlusSaveData.playerBodyAttribute - minAttributeValue;
        let reflexAttributeAdded: Int32 = this.m_ngPlusSaveData.playerReflexAttribute - minAttributeValue;
        let techAttributeAdded: Int32 = this.m_ngPlusSaveData.playerTechAttribute - minAttributeValue;
        let intAttributeAdded: Int32 = this.m_ngPlusSaveData.playerIntelligenceAttribute - minAttributeValue;
        let coolAttributeAdded: Int32 = this.m_ngPlusSaveData.playerCoolAttribute - minAttributeValue;

        this.m_startingAttributePoints = bodyAttributeAdded + reflexAttributeAdded + techAttributeAdded + intAttributeAdded + coolAttributeAdded;
    } else {
        this.m_startingAttributePoints = 7;
    }
    
    this.m_attributePointsAvailable = this.m_startingAttributePoints;
    inkTextRef
        .SetText(this.m_skillPointLabel, ToString(this.m_attributePointsAvailable));
    this.RefreshPointsLabel();
    this.ManageAllButtonsVisibility();
}
