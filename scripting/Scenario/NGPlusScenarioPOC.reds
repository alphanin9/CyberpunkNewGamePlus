// DO NOT MAKE THIS A MODULE, THAT BREAKS EVERYTHING
@addMethod(MenuScenario_SingleplayerMenu)
protected cb func OnNewGamePlus() -> Bool {
    this.CloseSubMenu();
    this.SwitchToScenario(n"MenuScenario_NewGamePlus");
}

public class MenuScenario_NewGamePlus extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        super.OnEnterScenario(prevScenario, userData);
        this.GetMenusState().OpenMenu(n"new_game_plus", userData);
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        super.OnLeaveScenario(nextScenario);
        this.GetMenusState().CloseMenu(n"new_game_plus");
    }

    protected cb func OnMainMenuBack() -> Bool {
        this.SwitchToScenario(this.m_prevScenario);
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_SelectNewGamePlusStart");
    }
}

public class MenuScenario_SelectNewGamePlusStart extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        super.OnEnterScenario(prevScenario, userData);
        this.GetMenusState().OpenMenu(n"new_game_plus_starting_point");
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        super.OnLeaveScenario(nextScenario);
        this.GetMenusState().CloseMenu(n"character_customization_background");
        this.GetMenusState().CloseMenu(n"new_game_plus_starting_point");
    }

    protected cb func OnMainMenuBack() -> Bool {
        this.SwitchToScenario(this.m_prevScenario);
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_NewGamePlusLifePathSelection");
    }
}

// Stuff to make as few wrapped methods as possible
public class MenuScenario_NewGamePlusLifePathSelection extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        super.OnEnterScenario(prevScenario, userData);
        let menuState = this.GetMenusState();
        menuState.OpenMenu(n"character_customization_background");
        menuState.OpenMenu(n"life_path_selection");
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        super.OnLeaveScenario(nextScenario);
        let menuState = this.GetMenusState();
        if NotEquals(nextScenario, n"MenuScenario_NewGamePlusBodyTypeSelection") {
            menuState.CloseMenu(n"character_customization_background");
        };
        menuState.CloseMenu(n"life_path_selection");
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_NewGamePlusBodyTypeSelection");
    }
}

public class MenuScenario_NewGamePlusBodyTypeSelection extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        super.OnEnterScenario(prevScenario, userData);
        this.GetMenusState().OpenMenu(n"gender_selection");
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        super.OnLeaveScenario(nextScenario);
        this.GetMenusState().CloseMenu(n"gender_selection");
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_NewGamePlusCharacterCustomization");
    }
}


public class MenuScenario_NewGamePlusCharacterCustomization extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        let morphMenuUserData: ref<MorphMenuUserData> = new MorphMenuUserData();
        morphMenuUserData.m_optionsListInitialized = Equals(prevScenario, n"MenuScenario_NewGamePlusStatsAdjustment");
        super.OnEnterScenario(prevScenario, userData);
        let menuState = this.GetMenusState();
        menuState.OpenMenu(n"player_puppet");
        menuState.OpenMenu(n"character_customization", morphMenuUserData);
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        super.OnLeaveScenario(nextScenario);
        let menuState = this.GetMenusState();
        if NotEquals(nextScenario, n"MenuScenario_NewGamePlusStatsAdjustment") {
            menuState.CloseMenu(n"player_puppet");
        };
        menuState.CloseMenu(n"character_customization");
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_NewGamePlusStatsAdjustment");
    }
}

// For later use for pregame stats adjustment (really cool idea actually)
public class MenuScenario_NewGamePlusStatsAdjustment extends MenuScenario_PreGameSubMenu {
    protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
        let menuState: wref<inkMenusState>;
        let statsMenuUserData: ref<StatsMenuUserData> = new StatsMenuUserData();
        statsMenuUserData.m_menuVisited = Equals(prevScenario, n"MenuScenario_Summary");
        super.OnEnterScenario(prevScenario, userData);
        menuState = this.GetMenusState();
        menuState.OpenMenu(n"player_puppet");
        menuState.OpenMenu(n"statistics_adjustment", statsMenuUserData);
    }

    protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
        let menuState: wref<inkMenusState>;
        super.OnLeaveScenario(nextScenario);
        menuState = this.GetMenusState();
        menuState.CloseMenu(n"statistics_adjustment");
    }

    protected cb func OnAccept() -> Bool {
        this.SwitchToScenario(n"MenuScenario_Summary");
    }
}