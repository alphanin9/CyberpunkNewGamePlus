module NewGamePlus.EP1Listener

public class NGPlusEP1StatusListener extends ScriptableSystem {
    public final func OnRestored(saveVersion: Int32, gameVersion: Int32) -> Void {
        let questsSystem = GameInstance.GetQuestsSystem(this.GetGameInstance());

        // This is only for NG+
        if questsSystem.GetFactStr("ngplus_active") == 0 {
            return;
        }

        let ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        let hasEp1 = IsEP1();

        if !hasEp1 {
            // We don't have EP1...
            return;
        }

        let ep1QuestActive = questsSystem.GetFactStr("ep1_installed") == 1;

        // If we have EP1 installed, but don't have ep1.quest loaded...
        if hasEp1 && !ep1QuestActive {
            ngPlusSystem.Spew("NGPlusEP1StatusListener::OnRestored: EP1 is installed but is not active, activating...");

            // Load the quest
            GameInstance.GetNewGamePlusSystem().LoadExpansionIntoSave();
        }
    }
}