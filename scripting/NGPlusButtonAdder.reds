module NGPlus.ButtonAdder

@wrapMethod(gameuiMenuItemListGameController)
private func AddMenuItem(const label: script_ref<String>, spawnEvent: CName) -> Void {
    wrappedMethod(label, spawnEvent);
    let singlePlayerMenu = this as SingleplayerMenuGameController;
    
    if !IsDefined(singlePlayerMenu) {
        return;
    }

    if Equals(ToString(label), GetLocalizedText("UI-ScriptExports-NewGame0")) {
        if !GameInstance.GetNewGamePlusSystem().HasPointOfNoReturnSave() {
            return;
        }
        wrappedMethod(GetLocalizedTextByKey(n"NewGamePlus_MainMenuButton"), n"OnNewGamePlus");
    }
}