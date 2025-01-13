module NGPlus.EP1Listener

import NGPlus.DifficultyConfig.*
import NGPlus.*

// General scriptable system-viable retrofixes
public class NGPlusEP1StatusListener extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;

    private final func RetrofixQ101SideContent() {
        // FIX: q101_enable_side_content going to 2 in Q101 start, thus potentially breaking shit...
        if this.m_questsSystem.GetFactStr("q101_enable_side_content") > 1 {
            this.m_questsSystem.SetFactStr("q101_enable_side_content", 1);
        }
    }

    public final static func ApplyRandomEncounterDisabler(questsSystem: ref<QuestsSystem>) {
        let factValue = 0;

        let settings = UserSettings.Get();

        if settings.enableRandomEncounters {
            factValue = 1;
        }

        questsSystem.SetFactStr("ngplus_enable_random_encounters", factValue);
    }

    private final func ActivateEP1() {
        let hasEp1 = IsEP1();

        if !hasEp1 {
            return;
        }

        let isEp1Loaded = this.m_questsSystem.GetFactStr("ep1_installed") > 0;

        if isEp1Loaded {
            return;
        }

        let ngPlusSystem = GameInstance.GetNewGamePlusSystem();

        // Fix: Encounter testing no longer triggers EP1 activation
        if ngPlusSystem.IsInNewGamePlusPrologue() || ngPlusSystem.IsInNewGamePlusHeistOrStandalone() {
            ngPlusSystem.Spew("EP1 installed, but not activated: loading...");
            ngPlusSystem.LoadExpansionIntoSave();
        }
    }

    public final func OnRestored(saveVersion: Int32, gameVersion: Int32) -> Void {
        this.m_questsSystem = GameInstance.GetQuestsSystem(this.GetGameInstance());

        let isNGPlus = this.m_questsSystem.GetFactStr("ngplus_active") == 1;
        let isStandalone = this.m_questsSystem.GetFactStr("ngplus_standalone_q101_start") == 1;

        // This is only for NG+
        if !isNGPlus && !isStandalone {
            return;
        }

        this.RetrofixQ101SideContent();
        this.ActivateEP1();

        if !isStandalone {
            NGPlusEP1StatusListener.ApplyRandomEncounterDisabler(this.m_questsSystem);
        }
    }
}

// Fix for a bug in Q001 start where `disable_tutorials` would mess parry tracking up
@replaceMethod(DefaultTransition)
protected const final func TutorialAddFact(scriptInterface: ref<StateGameScriptInterface>, factName: CName, add: Int32) -> Void {
    let val: Int32;
    let questSystem: ref<QuestsSystem> = scriptInterface.GetQuestsSystem();

    let newGamePlusSystem = GameInstance.GetNewGamePlusSystem();
    
    if questSystem.GetFact(n"disable_tutorials") == 0 || newGamePlusSystem.IsInNewGamePlusPrologue() {
        val = questSystem.GetFact(factName) + add;
        questSystem.SetFact(factName, val);
    }
}
