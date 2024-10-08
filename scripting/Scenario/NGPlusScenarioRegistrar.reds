module NGPlus.ScenarioRegistrar

class NewGamePlusMenuScenarioRegistrar extends ScriptableService {
    private cb func OnLoad() {
        GameInstance
            .GetCallbackSystem()
            .RegisterCallback(n"Resource/Ready", this, n"OnMenuResourceReady")
            .AddTarget(
                ResourceTarget
                    .Path(
                        r"base\\gameplay\\gui\\fullscreen\\main_menu\\pregame_menu.inkmenu"
                    )
            );
    }

    private final func GetWidgetEntry(widgetName: CName, widgetPath: String) -> inkMenuEntry {
        let widget = new ResourceRef();
        ResourceRef.LoadPath(widget, ResRef.FromString(widgetPath));

        let menuEntry: inkMenuEntry;

        menuEntry.name = widgetName;
        menuEntry.menuWidget = widget;
        menuEntry.depth = 100u;
        menuEntry.spawnMode = inkSpawnMode.SingleAndMultiplayer;
        menuEntry.isAffectedByFadeout = true;
        menuEntry.inputContext = n"";

        return menuEntry;
    }

    private final func GetNewGamePlusRegisteredScenarios() -> [CName] {
        let ret: [CName];

        ArrayPush(ret, n"MenuScenario_NewGamePlus");
        ArrayPush(ret, n"MenuScenario_SelectNewGamePlusStart");
        ArrayPush(ret, n"MenuScenario_NewGamePlusLifePathSelection");
        ArrayPush(ret, n"MenuScenario_NewGamePlusBodyTypeSelection");
        ArrayPush(ret, n"MenuScenario_NewGamePlusCharacterCustomization");
        ArrayPush(ret, n"MenuScenario_NewGamePlusStatsAdjustment");

        // Standalone Q101 without expansion... I guess?
        ArrayPush(ret, n"MenuScenario_NewGamePlusSelectStandaloneStartNonEP1");

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

        ArrayPush(
            resource.menusEntries,
            this
                .GetWidgetEntry(
                    n"new_game_plus",
                    "base\\gameplay\\gui\\fullscreen\\new_game_plus\\newgameplus_select_savegame.inkwidget"
                )
        );
        ArrayPush(
            resource.menusEntries,
            this
                .GetWidgetEntry(
                    n"new_game_plus_starting_point",
                    "base\\gameplay\\gui\\fullscreen\\new_game_plus\\newgameplus_select_starting_point.inkwidget"
                )
        );
        ArrayPush(
            resource.menusEntries,
            this
                .GetWidgetEntry(
                    n"new_game_plus_standalone_no_ep1",
                    "base\\gameplay\\gui\\fullscreen\\new_game_plus\\newgameplus_select_starting_point_standalone.inkwidget"
                )
        );
    }
}
