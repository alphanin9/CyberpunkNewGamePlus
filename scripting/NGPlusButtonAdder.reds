module NGPlus.ButtonAdder

@addField(SingleplayerMenuGameController)
public let m_hasNewGamePlus: Bool = false;

@addField(SingleplayerMenuGameController)
public let m_actionListMutex: RWLock;

@wrapMethod(SingleplayerMenuGameController)
protected cb func OnInitialize() -> Bool {
    wrappedMethod();

    let ngPlusSystem = GameInstance.GetNewGamePlusSystem();
    // Call our async callback
    ngPlusSystem.RequestHasValidNewGamePlusSaves(this, n"OnNewGamePlusSavesReady");
}

@addMethod(SingleplayerMenuGameController)
protected cb func OnNewGamePlusSavesReady(hasSaves: Bool) -> Void {
    if !hasSaves {
        let ngPlusSystem = GameInstance.GetNewGamePlusSystem();

        ngPlusSystem.Error("Could not find any saves meeting criteria!");
    }
    let hadNewGamePlus = this.m_hasNewGamePlus;

    this.m_hasNewGamePlus = hasSaves;

    if NotEquals(hadNewGamePlus, hasSaves) {
        this.ShowActionsList();
    }
}

@wrapMethod(SingleplayerMenuGameController)
protected func ShowActionsList() {
    // To avoid troubles with callbacks triggering simultaneously or almost simultaneously, add locking
    RWLock.Acquire(this.m_actionListMutex);
    wrappedMethod();
    RWLock.Release(this.m_actionListMutex);
}

@wrapMethod(gameuiMenuItemListGameController)
private func AddMenuItem(const label: script_ref<String>, spawnEvent: CName) -> Void {
    wrappedMethod(label, spawnEvent);

    let singlePlayerMenu = this as SingleplayerMenuGameController;

    if !IsDefined(singlePlayerMenu) {
        return;
    }

    if Equals(spawnEvent, n"OnNewGame") && singlePlayerMenu.m_hasNewGamePlus {
        wrappedMethod(GetLocalizedTextByKey(n"NewGamePlus_MainMenuButton"), n"OnNewGamePlus");
    }
}