module NGPlus.ExpansionNewGameAdditions

import NGPlus.SpawnTags.NewGamePlusSpawnTagController

@addField(ExpansionNewGame)
private let m_ngPlusStandaloneButton: inkWidgetRef;

@replaceMethod(ExpansionNewGame)
protected cb func OnInitialize() -> Bool {
    this.GetCharacterCustomizationSystem().InitializeState();
    super.OnInitialize();

    this.m_ngPlusStandaloneButton = inkWidgetRef.Create(this.GetWidget(inkWidgetPath.Build(n"ngplusQ101StandaloneButton")));

    inkWidgetRef.RegisterToCallback(this.m_baseGameButton, n"OnHoverOver", this, n"OnHoverOverBaseGame");
    inkWidgetRef.RegisterToCallback(this.m_baseGameButton, n"OnPress", this, n"OnPressBaseGame");
    inkWidgetRef.RegisterToCallback(this.m_standaloneButton, n"OnHoverOver", this, n"OnHoverOverExpansion");
    inkWidgetRef.RegisterToCallback(this.m_standaloneButton, n"OnPress", this, n"OnPressExpansion");
    inkWidgetRef.RegisterToCallback(this.m_creditsBase, n"OnHoverOver", this, n"OnHoverOverBaseCredits");
    inkWidgetRef.RegisterToCallback(this.m_creditsBase, n"OnHoverOut", this, n"OnHoverOutBaseCredits");
    inkWidgetRef.RegisterToCallback(this.m_creditsBase, n"OnPress", this, n"OnCredits");
    inkWidgetRef.RegisterToCallback(this.m_creditsExpansion, n"OnHoverOver", this, n"OnHoverOverExpansionCredits");
    inkWidgetRef.RegisterToCallback(this.m_creditsExpansion, n"OnHoverOut", this, n"OnHoverOutExpansionCredits");
    inkWidgetRef.RegisterToCallback(this.m_creditsExpansion, n"OnPress", this, n"OnCreditsEp1");
    inkWidgetRef.RegisterToCallback(this.m_ngPlusStandaloneButton, n"OnHoverOver", this, n"OnHoverOverNGPlusStandalone");
    inkWidgetRef.RegisterToCallback(this.m_ngPlusStandaloneButton, n"OnPress", this, n"OnPressNGPlusStandalone");
    this.OnIntro();
}

@wrapMethod(ExpansionNewGame)
protected cb func OnUninitialize() -> Bool {
    inkWidgetRef.UnregisterFromCallback(this.m_ngPlusStandaloneButton, n"OnHoverOver", this, n"OnHoverOverNGPlusStandalone");
    inkWidgetRef.UnregisterFromCallback(this.m_ngPlusStandaloneButton, n"OnPress", this, n"OnPressNGPlusStandalone");

    wrappedMethod();
}

@wrapMethod(ExpansionNewGame)
protected cb func OnIntroComplete(anim: ref<inkAnimProxy>) -> Bool {
    wrappedMethod(anim);

    inkWidgetRef.SetInteractive(this.m_ngPlusStandaloneButton, true);
}

@wrapMethod(ExpansionNewGame)
protected func PriorMenu() -> Void {
    // Just in case...
    let newGamePlusSystem = GameInstance.GetNewGamePlusSystem();

    newGamePlusSystem.SetNewGamePlusState(false);
    newGamePlusSystem.SetStandaloneState(false);

    NewGamePlusSpawnTagController.RestoreSpawnTags();
    wrappedMethod();
}

@addMethod(ExpansionNewGame)
protected cb func OnHoverOverNGPlusStandalone(evt: ref<inkPointerEvent>) -> Bool {
    this.TextureTransition(n"flow_base_game");

    this.localizedText = GetLocalizedTextByKey(n"NewGamePlus_PostHeistStartStandalone_Desc");
    this.translationAnimationCtrl.SetBaseText("FF:06:B5");
    this.translationAnimationCtrl = inkWidgetRef.GetController(this.m_newGameDescription) as inkTextReplaceController;
    this.translationAnimationCtrl.SetTargetText(this.localizedText);
    this.translationAnimationCtrl.PlaySetAnimation();
}

@wrapMethod(ExpansionNewGame)
protected cb func OnPressBaseGame(evt: ref<inkPointerEvent>) -> Bool {
    let newGamePlusSystem = GameInstance.GetNewGamePlusSystem();

    newGamePlusSystem.SetNewGamePlusState(false);
    newGamePlusSystem.SetStandaloneState(false);

    NewGamePlusSpawnTagController.RestoreSpawnTags();
    wrappedMethod(evt);
}

@wrapMethod(ExpansionNewGame)
protected cb func OnPressExpansion(evt: ref<inkPointerEvent>) -> Bool {
    let newGamePlusSystem = GameInstance.GetNewGamePlusSystem();

    newGamePlusSystem.SetNewGamePlusState(false);
    newGamePlusSystem.SetStandaloneState(false);

    NewGamePlusSpawnTagController.RestoreSpawnTags();
    wrappedMethod(evt);
}

@addMethod(ExpansionNewGame)
protected cb func OnPressNGPlusStandalone(evt: ref<inkPointerEvent>) -> Bool {
    if evt.IsAction(n"click") && !this.m_isInputLocked {
        this.PlaySound(n"Button", n"OnPress");

        let newGamePlusSystem = GameInstance.GetNewGamePlusSystem();

        newGamePlusSystem.SetNewGamePlusState(true);
        newGamePlusSystem.SetStandaloneState(true);

        this.m_characterCustomizationState.SetIsExpansionStandalone(false);

        newGamePlusSystem.SetNewGamePlusGameDefinition(ENewGamePlusStartType.StartFromQ101_ProgressionBuild);

        NewGamePlusSpawnTagController.SetSpawnTags(n"#q101_spwn_player");

        this.NextMenu();
    }
}