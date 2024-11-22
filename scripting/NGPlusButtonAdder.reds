module NGPlus.ButtonAdder



@wrapMethod(gameuiMenuItemListGameController)
private func AddMenuItem(const label: script_ref<String>, spawnEvent: CName) -> Void {
    wrappedMethod(label, spawnEvent);

    // Assuming no modifications, the only call with OnNewGame as an arg should be from singleplayer menu
    if Equals(spawnEvent, n"OnNewGame") {
        // Make this async and recall PopulateMenuItems
        if !GameInstance.GetNewGamePlusSystem().HasPointOfNoReturnSave() {
            return;
        }
        wrappedMethod(GetLocalizedTextByKey(n"NewGamePlus_MainMenuButton"), n"OnNewGamePlus");
    }
}