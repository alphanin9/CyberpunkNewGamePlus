
@replaceMethod(characterCreationSummaryMenu)
protected cb func OnOutroComplete(anim: ref<inkAnimProxy>) {
    let ccSystem = this.GetCharacterCustomizationSystem();
    ccSystem.FinalizeState();

    let ngPlusSystem = GameInstance.GetNewGamePlusSystem();

    let ngPlusQuest = ngPlusSystem.GetNewGamePlusQuest();

    if NotEquals(ngPlusQuest, ENGPlusType.Count) && NotEquals(ngPlusQuest, ENGPlusType.Invalid) {
        // Fairly safe cast, no other derivatives
        ngPlusSystem.Spew(s"Starting NG+ playthrough with quest type \(ngPlusQuest)...");
        ngPlusSystem
            .LaunchNewGamePlus(
                this.m_characterCustomizationState as gameuiCharacterCustomizationState
            );
    } else {
        this
            .GetSystemRequestsHandler()
            .StartNewGame(this.m_characterCustomizationState);
        if this.m_characterCustomizationState.IsExpansionStandalone() {
            this.GetTelemetrySystem().LogPlaythroughEp1();
        }
    }

    this
        .GetTelemetrySystem()
        .LogInitialChoiceSetStatege(telemetryInitalChoiceStage.Finished);
    this.GetTelemetrySystem().LogNewGameStarted();
    super.NextMenu();
}
