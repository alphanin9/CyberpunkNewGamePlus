module NGPlus.Recipes

// This is kinda eh behavior, but it should work?
// I think it should be so it gets hidden if we craft the mod/gun in question, but shouldn't be hidden if it's the other mods

@addMethod(CraftBook)
private final func GetForceUnhiddenChimeraModRecipes() -> [TweakDBID] = [t"Items.ChimeraPowerMod_Recipe", t"Items.ChimeraTechMod_Recipe", t"Items.ChimeraSmartMod_Recipe", t"Items.ChimeraMeleeMod_Recipe"];

@addMethod(CraftBook)
private final func GetNotIgnoredChimeraPowerModRecipe() -> TweakDBID = t"Items.ChimeraPowerMod";

@addMethod(CraftBook)
private final func GetNotIgnoredChimeraTechModRecipe() -> TweakDBID = t"Items.ChimeraTechMod";

@addMethod(CraftBook)
private final func GetForceUnhiddenHauntedRecipes() -> [TweakDBID] = [t"Items.Recipe_HauntedCyberdeck", t"Items.Recipe_Rare_HauntedCyberdeck", t"Items.Recipe_Epic_HauntedCyberdeck", t"Items.Recipe_Legendary_HauntedCyberdeck", t"Items.Recipe_Common_Borg4a_HauntedGun", t"Items.Recipe_Uncommon_Borg4a_HauntedGun", t"Items.Recipe_Rare_Borg4a_HauntedGun", t"Items.Recipe_Epic_Borg4a_HauntedGun", t"Items.Recipe_Legendary_Borg4a_HauntedGun"];

// Slow code, but meh
@addMethod(CraftBook)
private final func IsChimeraMod(itemId: TweakDBID) -> Bool {
    let modList = [
        t"Items.ChimeraPowerMod", 
        t"Items.ChimeraTechMod", 
        t"Items.ChimeraSmartMod", 
        t"Items.ChimeraMeleeMod"
    ];

    return ArrayContains(modList, itemId);
}

@addMethod(CraftBook)
private final func IsHauntedItem(itemId: TweakDBID) -> Bool {
    let hauntedList = [
        t"Items.HauntedCyberdeck_Rare", 
        t"Items.HauntedCyberdeck_Epic", 
        t"Items.HauntedCyberdeck_Legendary", 
        t"Items.Common_Borg4a_HauntedGun", 
        t"Items.Uncommon_Borg4a_HauntedGun", 
        t"Items.Rare_Borg4a_HauntedGun", 
        t"Items.Epic_Borg4a_HauntedGun", 
        t"Items.Legendary_Borg4a_HauntedGun"
    ];

    return ArrayContains(hauntedList, itemId);
}

// Inefficient code
@replaceMethod(CraftBook)
public final func HideRecipesForOwnedItems() -> Void {
    let craftingSystem = CraftingSystem.GetInstance(this.m_owner.GetGame());
    let questsSystem = GameInstance.GetQuestsSystem(this.m_owner.GetGame());
    let transactionSystem = GameInstance.GetTransactionSystem(this.m_owner.GetGame());

    let isNgPlusActive = questsSystem.GetFactStr("ngplus_active") == 1;

    let i = 0;

    while i < ArraySize(this.m_knownRecipes) {
        let shouldBeUnhidden = false;
        let canBeHidden = ArraySize(this.m_knownRecipes[i].hideOnItemsAdded) > 0;
        
        if isNgPlusActive && canBeHidden {
            let resultItem = this.m_knownRecipes[i].targetItem;

            // The only ones we care about...
            if this.IsChimeraMod(resultItem) || this.IsHauntedItem(resultItem) {
                let hasItem = false;

                let record = TweakDBInterface.GetItemRecord(resultItem);

                if craftingSystem.CanItemBeCrafted(record) {
                    for itemToHideOn in this.m_knownRecipes[i].hideOnItemsAdded {
                        if Equals(ItemID.GetTDBID(itemToHideOn), resultItem) {
                            if transactionSystem.HasItem(this.m_owner, itemToHideOn) {
                                hasItem = true;
                                break;
                            }
                        }
                    }

                    this.m_knownRecipes[i].isHidden = hasItem;
                    shouldBeUnhidden = !hasItem;
                } else {
                    // We can't craft it with our current resources, don't bother...
                    this.m_knownRecipes[i].isHidden = true;
                }
            }
        }

        if !this.m_knownRecipes[i].isHidden && canBeHidden && !shouldBeUnhidden {
            for itemToHideOn in this.m_knownRecipes[i].hideOnItemsAdded {
                if transactionSystem.HasItem(this.m_owner, itemToHideOn) {
                    this.m_knownRecipes[i].isHidden = true;
                    break;
                };
            }
        };
        i += 1;
    }
}