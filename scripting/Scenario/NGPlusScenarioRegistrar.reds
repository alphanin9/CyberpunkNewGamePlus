module NGPlus.ScenarioRegistrar

class NewGamePlusMenuScenarioRegistrar extends ScriptableService {
    private cb func OnLoad() {
        GameInstance.GetCallbackSystem().RegisterCallback(n"Resource/Ready", this, n"OnMenuResourceReady")
            .AddTarget(ResourceTarget.Path(r"base\\gameplay\\gui\\fullscreen\\main_menu\\pregame_menu.inkmenu"));
    }

    private final func GetNewGamePlusMenuEntry() -> inkMenuEntry {
        let name = n"new_game_plus";
        let widget = new ResourceRef();
        ResourceRef.LoadPath(widget, ResRef.FromString("base\\gameplay\\gui\\fullscreen\\new_game_plus\\newgameplus_select_savegame.inkwidget"));

        let depth: Uint32 = 100u;
        let spawnMode = inkSpawnMode.SingleAndMultiplayer;
        let isAffectedByFadeout = true;
        let inputContext = n"";

        return new inkMenuEntry(
            name,
            widget,
            depth,
            spawnMode,
            isAffectedByFadeout,
            inputContext
        );
    }

    private final func GetNewGamePlusSelectStartMenuEntry() -> inkMenuEntry {
        let name = n"new_game_plus_starting_point";
        let widget = new ResourceRef();
        ResourceRef.LoadPath(widget, ResRef.FromString("base\\gameplay\\gui\\fullscreen\\new_game_plus\\newgameplus_select_starting_point.inkwidget"));

        let depth: Uint32 = 100u;
        let spawnMode = inkSpawnMode.SingleAndMultiplayer;
        let isAffectedByFadeout = true;
        let inputContext = n"";

        return new inkMenuEntry(
            name,
            widget,
            depth,
            spawnMode,
            isAffectedByFadeout,
            inputContext
        );
    }

    private final func GetNewGamePlusRegisteredScenarios() -> array<CName> {
        let ret: array<CName>;

        ArrayPush(ret, n"MenuScenario_NewGamePlus");
        ArrayPush(ret, n"MenuScenario_SelectNewGamePlusStart");
        ArrayPush(ret, n"MenuScenario_NewGamePlusLifePathSelection");
        ArrayPush(ret, n"MenuScenario_NewGamePlusBodyTypeSelection");
        ArrayPush(ret, n"MenuScenario_NewGamePlusCharacterCustomization");
        ArrayPush(ret, n"MenuScenario_NewGamePlusStatsAdjustment");

        return ret;
    }

    private cb func OnMenuResourceReady(event: ref<ResourceEvent>) {
        let resource = event.GetResource() as inkMenuResource;

        if !IsDefined(resource) {
            return;
        }

        let scenarios = this.GetNewGamePlusRegisteredScenarios();

        for scenario in scenarios {
            ArrayPush(resource.scenariosNames, scenario);
        }
        
        //ArrayPush(resource.scenariosNames, n"MenuScenario_NewGamePlus");
        //ArrayPush(resource.scenariosNames, n"MenuScenario_SelectNewGamePlusStart");

        ArrayPush(resource.menusEntries, this.GetNewGamePlusMenuEntry());
        ArrayPush(resource.menusEntries, this.GetNewGamePlusSelectStartMenuEntry());
    }
}