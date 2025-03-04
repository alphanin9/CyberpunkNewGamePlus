public class NewGamePlusStartingPointController extends BaseCharacterCreationController {
    public let m_newGameDescription: inkTextRef;
    public let m_textureTop: inkImageRef;
    public let m_textureBottom: inkImageRef;
    private let m_introAnimation: CName;
    private let m_outroAnimation: CName;
    private let m_hoverAnimation: CName;
    private let m_animationProxy: ref<inkAnimProxy>;
    private let translationAnimationCtrl: wref<inkTextReplaceController>;
    private let localizedText: String;
    private let m_lastShownPart: CName;
    private let m_q001Button: inkWidgetRef;
    private let m_q101Button: inkWidgetRef;

    private let m_ep1Enabled: Bool;
    private let m_isInputLocked: Bool;

    private let m_ngPlusSystem: ref<NewGamePlusSystem>;

	private final func InitializeUninitializableWKitVariables() {
		this.m_hoverAnimation = n"hoverAnimation";
		this.m_introAnimation = n"intro";
		this.m_outroAnimation = n"outro";
	}

    protected cb func OnInitialize() -> Bool {
        super.OnInitialize();

		this.InitializeUninitializableWKitVariables();
        this.GetCharacterCustomizationSystem().InitializeState();
        inkWidgetRef.RegisterToCallback(this.m_q001Button, n"OnHoverOver", this, n"OnHoverOverQ001");
        inkWidgetRef.RegisterToCallback(this.m_q001Button, n"OnPress", this, n"OnPressQ001");
        inkWidgetRef.RegisterToCallback(this.m_q101Button, n"OnHoverOver", this, n"OnHoverOverQ101");
        inkWidgetRef.RegisterToCallback(this.m_q101Button, n"OnPress", this, n"OnPressQ101");

        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();

        // Select the correct game definition based on the person owning EP1 or not, so nothing ends up broken in EP1-free scenarios...
        this.m_ep1Enabled = this.GetSystemRequestsHandler().IsAdditionalContentEnabled(n"EP1");

        this.OnIntro();
    }

    protected cb func OnUninitialize() -> Bool {
        super.OnUninitialize();
        inkWidgetRef
            .UnregisterFromCallback(this.m_q001Button, n"OnHoverOver", this, n"OnHoverOverQ001");
        inkWidgetRef
            .UnregisterFromCallback(this.m_q001Button, n"OnPress", this, n"OnPressQ001");
        inkWidgetRef
            .UnregisterFromCallback(
                this.m_q101Button,
                n"OnHoverOver",
                this,
                n"OnHoverOverQ101"
            );
        inkWidgetRef
            .UnregisterFromCallback(this.m_q101Button, n"OnPress", this, n"OnPressQ101");
    }

    private final func OnSelectedOption() {
        // Telemetry stuff, this is mandatory apparently?
        this.GetTelemetrySystem().LogInitialChoiceSetStatege(telemetryInitalChoiceStage.Difficulty);
        GameInstance.GetStatsDataSystem(this.GetPlayerControlledObject().GetGame()).SetDifficulty(gameDifficulty.VeryHard);
        this.GetTelemetrySystem().LogInitialChoiceDifficultySelected(gameDifficulty.VeryHard);

        this.NextMenu();
    }

    protected cb func OnPressQ001(evt: ref<inkPointerEvent>) -> Bool {
        if evt.IsAction(n"click") && !this.m_isInputLocked {
            this.PlaySound(n"Button", n"OnPress");
            this.m_ngPlusSystem.SetNewGamePlusQuest(ENGPlusType.StartFromQ001);
            this.OnSelectedOption();
        }
    }

    protected cb func OnPressQ101(evt: ref<inkPointerEvent>) -> Bool {
        if evt.IsAction(n"click") && !this.m_isInputLocked {
            this.PlaySound(n"Button", n"OnPress");
            this.m_ngPlusSystem.SetNewGamePlusQuest(ENGPlusType.StartFromQ101);
            this.OnSelectedOption();
        }
    }

    private final func TextureTransition(part: CName) -> Void {
        if NotEquals(this.m_lastShownPart, part) {
            this.m_lastShownPart = part;
            inkImageRef.SetTexturePart(
                    this.m_textureBottom,
                    inkImageRef.GetTexturePart(this.m_textureTop)
                );
            inkWidgetRef.SetOpacity(this.m_textureTop, 0.0);
            inkImageRef.SetTexturePart(this.m_textureTop, part);
            this.PlayLibraryAnimation(n"hoverAnimation");
        }
    }

    protected cb func OnHoverOverQ001(e: ref<inkPointerEvent>) -> Bool {
        this.TextureTransition(n"flow_ngplus_q001");
        this.localizedText = GetLocalizedTextByKey(n"NewGamePlus_Q001Start_Description");
        this.translationAnimationCtrl.SetBaseText("FF:06:B5");
        this.translationAnimationCtrl = inkWidgetRef.GetController(this.m_newGameDescription) as inkTextReplaceController;
        this.translationAnimationCtrl.SetTargetText(this.localizedText);
        this.translationAnimationCtrl.PlaySetAnimation();
    }

    protected cb func OnHoverOverQ101(e: ref<inkPointerEvent>) -> Bool {
        this.TextureTransition(n"flow_ngplus_q101");
        this.localizedText = GetLocalizedTextByKey(n"NewGamePlus_Q101Start_Description");
        this.translationAnimationCtrl.SetBaseText("FF:06:B5");
        this.translationAnimationCtrl = inkWidgetRef.GetController(this.m_newGameDescription) as inkTextReplaceController;
        this.translationAnimationCtrl.SetTargetText(this.localizedText);
        this.translationAnimationCtrl.PlaySetAnimation();
    }

    protected func PriorMenu() -> Void {
        this.GetTelemetrySystem().LogInitialChoiceSetStatege(telemetryInitalChoiceStage.None);
        this.GetCharacterCustomizationSystem().ClearState();
        super.PriorMenu();
    }

    protected func NextMenu() -> Void {
        this.m_isInputLocked = true;
        this.OnOutro();
    }

    private final func OnIntro() -> Void {
        this.PlayAnim(this.m_introAnimation, n"OnIntroComplete");
        this.PlaySound(n"CharacterCreationConfirmationAnimation", n"OnOpen");
    }

    private final func OnOutro() -> Void {
        let animOptions: inkAnimOptions;
        animOptions.playReversed = false;
        this.PlayAnim(this.m_outroAnimation, n"OnOutroComplete", animOptions);
    }

    protected cb func OnOutroComplete(anim: ref<inkAnimProxy>) -> Bool {
        super.NextMenu();
    }

    protected cb func OnIntroComplete(anim: ref<inkAnimProxy>) -> Bool {
        inkWidgetRef.SetInteractive(this.m_q001Button, true);
        inkWidgetRef.SetInteractive(this.m_q101Button, true);
    }

    public final func PlayAnim(animName: CName, opt callBack: CName, opt options: inkAnimOptions) -> Void {
        if IsDefined(this.m_animationProxy) && this.m_animationProxy.IsPlaying() {
            this.m_animationProxy.Stop();
        }
        this.m_animationProxy = this.PlayLibraryAnimation(animName, options);
        if NotEquals(callBack, n"None") {
            this.m_animationProxy.RegisterToCallback(inkanimEventType.OnFinish, this, callBack);
        }
    }
}
