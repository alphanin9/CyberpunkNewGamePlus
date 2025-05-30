public class NewGamePlusSelectionController extends gameuiSaveHandlingController {
    private let m_list: inkCompoundRef;
    private let m_noSavedGamesLabel: inkWidgetRef;
    private let m_buttonHintsManagerRef: inkWidgetRef;
    private let m_transitToLoadingAnimName: CName;
    private let m_transitToLoadingSlotAnimName: CName;
    private let m_animDelayBetweenSlots: Float;
    private let m_animDelayForMainSlot: Float;
    private let m_enableLoadingTransition: Bool;
    private let m_laodingSpinner: inkWidgetRef;
    private let m_scrollbar: inkWidgetRef;
    private let m_eventDispatcher: wref<inkMenuEventDispatcher>;
    private let m_loadComplete: Bool;
    private let m_saveInfo: ref<SaveMetadataInfo>;
    private let m_buttonHintsController: wref<ButtonHints>;
    private let m_saveToLoadIndex: Int32;
    private let m_saveToLoadID: Uint64;
    private let m_isInputDisabled: Bool;
    private let m_saves: [String];
    private let m_ngPlusSaveIndices: [Int32];
    private let m_saveFilesReady: Bool;
    private let m_systemHandler: wref<inkISystemRequestsHandler>;
    private let m_pendingRegistration: Bool;
    private let m_isEp1Enabled: Bool;
    private let m_animProxy: ref<inkAnimProxy>;
    private let m_sourceIndex: Int32;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;
    private let m_selectedLoadListItem: wref<LoadListItem>;

    // WolvenKit only lets me specify *some* things, but inkwidgets generally are just enough
    private final func InitializeUninitializableWKitVariables() {
        this.m_animDelayBetweenSlots = 0.150000006;
        this.m_animDelayForMainSlot = 1;
        this.m_transitToLoadingAnimName = n"TransitionToLoadingScreen";
        this.m_transitToLoadingSlotAnimName = n"saves_fade_out_v02";
        this.m_enableLoadingTransition = true;

        this.m_isEp1Enabled = false;
        this.m_loadComplete = false;
        this.m_isInputDisabled = false;
        this.m_sourceIndex = 0;
        this.m_saveToLoadID = 0u;
        this.m_saveToLoadIndex = 0;
        this.m_pendingRegistration = false;
    }

    protected cb func OnInitialize() -> Bool {
        this.InitializeUninitializableWKitVariables();
        
        this.m_systemHandler = this.GetSystemRequestsHandler();
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        this.m_ngPlusSystem.SetNewGamePlusQuest(ENGPlusType.Invalid);
        this
            .m_systemHandler
            .RegisterToCallback(n"OnSavesForLoadReady", this, n"OnSavesForLoadReady");
        this
            .m_systemHandler
            .RegisterToCallback(n"OnSaveMetadataReady", this, n"OnSaveMetadataReady");
        this.m_systemHandler.RequestSavesForLoad();
        this
            .RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
        this.PlayLibraryAnimation(n"intro");
        this.m_buttonHintsController = this
            .SpawnFromExternal(
                inkWidgetRef.Get(this.m_buttonHintsManagerRef),
                r"base\\gameplay\\gui\\common\\buttonhints.inkwidget",
                n"Root"
            )
            .GetController() as ButtonHints;
        this.UpdateButtonHints(1);
        this.m_isInputDisabled = false;
        this.PlayLoadingAnimation();
        this.m_isEp1Enabled = IsEP1();
    }

    protected cb func OnUninitialize() -> Bool {
        this.UnregisterFromGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
    }

    protected cb func OnButtonRelease(evt: ref<inkPointerEvent>) -> Bool {
        if evt.IsAction(n"back") {
            this.m_ngPlusSystem.SetNewGamePlusQuest(ENGPlusType.Invalid);
            this.m_eventDispatcher.SpawnEvent(n"OnMainMenuBack");
        }
    }

    private final func UpdateButtonHints(savesCount: Int32) {
        this.m_buttonHintsController.ClearButtonHints();
        this.m_buttonHintsController.AddButtonHint(n"back", GetLocalizedText("Common-Access-Close"));
        if savesCount > 0 {
            this.m_buttonHintsController.AddButtonHint(n"select", GetLocalizedText("UI-UserActions-Select"));
        };
    }

    private final func SetupLoadItems() -> Void {
        for i in this.m_ngPlusSaveIndices {
            this.CreateLoadItem(i);
        }
    }

    private final func CreateLoadItem(index: Int32) -> Void {
        let currButton: wref<inkCompoundWidget> = this.SpawnFromLocal(inkWidgetRef.Get(this.m_list), n"LoadListItem") as inkCompoundWidget;
        currButton.RegisterToCallback(n"OnRelease", this, n"OnRelease");
        let currLogic: wref<LoadListItem> = currButton.GetController() as LoadListItem;
        currLogic.SetData(index);
        this.GetSystemRequestsHandler().RequestSavedGameScreenshot(index, currLogic.GetPreviewImageWidget());
    }

    private final func PlayLoadingAnimation() {
        let i = 0;
        inkCompoundRef.RemoveAllChildren(this.m_list);

        while i < 7 {
            this.SpawnFromLocal(inkWidgetRef.Get(this.m_list), n"LoadListItemPlaceholder");
            i += 1;
        }
        inkWidgetRef.SetVisible(this.m_scrollbar, false);
    }

    private final func StopLoadingAnimation() {
        inkWidgetRef.SetVisible(this.m_scrollbar, true);
    }

    protected cb func OnRelease(e: ref<inkPointerEvent>) -> Bool {
        if !this.m_isInputDisabled {
            let button: wref<inkWidget> = e.GetCurrentTarget();
            let controller: wref<LoadListItem> = button.GetController() as LoadListItem;

            if e.IsAction(n"click") && Equals(this.m_loadComplete, true) {
                this.PlaySound(n"Button", n"OnPress");
                if controller.ValidSlot() {
                    this.OnSelectedSave(controller);
                }
            }
        }
    }

    // Did we succeed at save loader?
    private cb func OnNewGamePlusSaveLoaded(result: Bool) {
        if !result {
            let controller: ref<LoadListItem> = this.m_selectedLoadListItem;

            if IsDefined(controller) {
                // NOTE: add localization? Better error messages?
                controller.SetInvalid("Failed to parse progression data!\nTry going into a game and saving again.");
                return;
            }
        }

        this.m_eventDispatcher.SpawnEvent(n"OnAccept");
    }

    private final func OnSelectedSave(controller: ref<LoadListItem>) -> Void {
        // Store used controller for error reporting
        // Then tell NG+ system to process the save
        this.m_selectedLoadListItem = controller;
        this.m_ngPlusSystem.RequestLoadSaveData(this.m_saves[controller.Index()], this, n"OnNewGamePlusSaveLoaded");
    }

    protected cb func OnSavesForLoadReady(saves: [String]) -> Bool {
        this.m_ngPlusSystem.RequestResolveNewGamePlusSaves(saves, this, n"OnNewGamePlusSaveResolveRead");
        this.m_saves = saves;
    }

    protected cb func OnNewGamePlusSaveResolveRead(saveIds: [Int32]) {
        this.m_saveFilesReady = true;
        this.m_ngPlusSaveIndices = saveIds;

        this.UpdateSavesList();
    }

    private final func UpdateSavesList() -> Void {
        if this.m_saveFilesReady {
            this.m_saveFilesReady = false;
            this.StopLoadingAnimation();
            inkCompoundRef.RemoveAllChildren(this.m_list);
            this.SetupLoadItems();
            let savesCount = ArraySize(this.m_ngPlusSaveIndices);
            inkWidgetRef.SetVisible(this.m_noSavedGamesLabel, savesCount == 0);
            this.UpdateButtonHints(savesCount);
            this.m_loadComplete = true;
        };
    }

    protected cb func OnSaveMetadataReady(info: ref<SaveMetadataInfo>) -> Bool {
        let characterCustomizationSystem = GameInstance.GetCharacterCustomizationSystem(this.GetPlayerControlledObject().GetGame());
        let i = 0;
        while i < inkCompoundRef.GetNumChildren(this.m_list) {
            let button: wref<inkWidget> = inkCompoundRef.GetWidgetByIndex(this.m_list, i);
            let controller: wref<LoadListItem> = button.GetController() as LoadListItem;
            if controller.Index() == info.saveIndex {
                // Platform check is moved into native code now
                if info.isValid {
                    controller.SetMetadataForNGPlus(info, this.m_isEp1Enabled);
                    controller.CheckThumbnailCensorship(!characterCustomizationSystem.IsNudityAllowed());
                } else {
                    controller.SetInvalid(info.internalName);
                };
                break;
            };
            i += 1;
        };
    }

    protected cb func OnSetMenuEventDispatcher(menuEventDispatcher: wref<inkMenuEventDispatcher>) -> Bool {
        this.m_eventDispatcher = menuEventDispatcher;
    }
}
