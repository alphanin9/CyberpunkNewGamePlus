module EquipmentEx

public class EquipmentExTransfer {
    private let m_data: ref<NGPlusProgressionData>;
    private let m_ngPlusSystem: ref<NewGamePlusSystem>;

    public final func Init() {
        this.m_ngPlusSystem = GameInstance.GetNewGamePlusSystem();
        this.m_data = this.m_ngPlusSystem.GetProgressionData();
    }

    public static func Make() -> ref<EquipmentExTransfer> {
        let obj = new EquipmentExTransfer();

        obj.Init();

        return obj;
    }

    @if(ModuleExists("EquipmentEx"))
    public final func TransferEquipmentEx() {
        let viewManagerResults = this.m_data.GetEquipmentExViewManagerResults();

        if IsDefined(viewManagerResults) {            
            let viewManager: ref<ViewManager> = ViewManager.GetInstance(GetGameInstance());
            
            switch(viewManagerResults.GetSource()) {
                case 0ul:
                    viewManager.SetItemSource(WardrobeItemSource.WardrobeStore);
                    break;
                case 1ul:
                    viewManager.SetItemSource(WardrobeItemSource.InventoryAndStash);
                    break;
                case 2ul:
                    viewManager.SetItemSource(WardrobeItemSource.InventoryOnly);
                    break;
                default:
                    break;
            }
        }

        let outfitManagerResults = this.m_data.GetEquipmentExOutfitSystemResults();

        let outfitSystem = OutfitSystem.GetInstance(GetGameInstance());

        let state = outfitSystem.m_state;

        if IsDefined(outfitManagerResults) {
            for outfit in outfitManagerResults.GetData() {
                let outfitName = outfit.GetName();
                let outfitParts: [ref<OutfitPart>];

                for outfitPart in outfit.GetOutfitParts() {
                    ArrayPush(outfitParts, OutfitPart.Create(outfitPart.GetItemID(), outfitPart.GetSlotID()));    
                }

                let timestamp = EngineTime.ToFloat(GameInstance.GetPlaythroughTime(GetGameInstance()));

                state.SaveOutfit(outfitName, outfitParts, false, timestamp);
            }
        }
    }

    @if(!ModuleExists("EquipmentEx"))
    public final func TransferEquipmentEx() {
        this.m_ngPlusSystem.Spew("EquipmentExTransfer::TransferEquipmentEx, EquipmentEx is not present!");
    }
}

