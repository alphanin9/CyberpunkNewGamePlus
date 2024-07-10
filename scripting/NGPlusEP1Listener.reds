module NewGamePlus.EP1Listener

// General scriptable system-viable retrofixes, old filename is kept for manual installs...
public class NGPlusEP1StatusListener extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;

    private final func RetrofixQ101SideContent() {
        // FIX: q101_enable_side_content going to 2 in Q101 start, thus potentially breaking shit...
        if this.m_questsSystem.GetFactStr("q101_enable_side_content") > 1 {
            this.m_questsSystem.SetFactStr("q101_enable_side_content", 1);
        }
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

        ngPlusSystem.Spew("EP1 installed, but not activated: loading...");
        ngPlusSystem.LoadExpansionIntoSave();
    }

    public final func OnRestored(saveVersion: Int32, gameVersion: Int32) -> Void {
        this.m_questsSystem = GameInstance.GetQuestsSystem(this.GetGameInstance());

        // This is only for NG+
        if this.m_questsSystem.GetFactStr("ngplus_active") == 0 {
            return;
        }

        this.RetrofixQ101SideContent();
        this.ActivateEP1();
    }
}