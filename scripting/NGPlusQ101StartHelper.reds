module NGPlus.Q101StartHelper

import NGPlus.DifficultyConfig.*

// Hacky way to do this, but no real other way...
public class NGPlusQ101Helper extends ScriptableSystem {
    private let m_questsSystem: ref<QuestsSystem>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_newQ101ListenerId: Uint32;

    private final func OnAttach() -> Void {
        this.m_questsSystem = GameInstance.GetQuestsSystem(GetGameInstance());
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        
        this.m_newQ101ListenerId = this.m_questsSystem.RegisterListener(n"ngplus_use_new_q101", this, n"OnFactChange");
    }

    public final func OnFactChange(newValue: Int32) -> Void {
        if newValue != 1 || !GetShouldFastForwardQ101Start() {
            return;
        }

        if this.m_questsSystem.GetFactStr("ngplus_active") == 0 {
            this.m_ngPlusSystem.Spew("NGPlusQ101Helper::OnFactChange, NG+ inactive...");
            return;
        }

        let isQ101Start = this.m_questsSystem.GetFactStr("ngplus_q001_start") == 0;

        if isQ101Start {
            this.m_ngPlusSystem.Spew("NGPlusQ101Helper::OnFactChange, should fast-forward to V's room...");
            this.m_questsSystem.SetFactStr("ngplus_use_new_q101", 0);
        }
    }

    private final func OnDetach() -> Void {
        this.m_questsSystem.UnregisterListener(n"ngplus_use_new_q101", this.m_newQ101ListenerId);
    }
}