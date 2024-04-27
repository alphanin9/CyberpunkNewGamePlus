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
        this.SwitchToScenario(n"MenuScenario_LifePathSelection");
    }
}