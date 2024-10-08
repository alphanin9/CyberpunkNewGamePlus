module NGPlus.CraftBook

@addMethod(CraftBook)
public final func AddRecipeFromInfo(craftInfo: ref<NGPlusCraftingInfo>) {
    let targetTdbid = craftInfo.GetTargetItem();
    let amount = craftInfo.GetAmount();

    if this.KnowsRecipe(targetTdbid) || !TDBID.IsValid(targetTdbid) {
        return;
    }

    let recipe: ItemRecipe;

    recipe.targetItem = targetTdbid;

    if amount > 0 && amount != 1 {
        recipe.amount = amount;
    } else {
        recipe.amount = 1;
    };
    
    let hideOnItemsAdded = craftInfo.GetHideOnItemsAdded();

    if ArraySize(hideOnItemsAdded) > 0 {
        let transactionSystem = GameInstance.GetTransactionSystem(this.m_owner.GetGame());

        for item in hideOnItemsAdded {
            ArrayPush(recipe.hideOnItemsAdded, item);

            if transactionSystem.HasItem(this.m_owner, item) {
                recipe.isHidden = true;
            }
        }
    }

    let recipeId = this.GetRecipeIndex(targetTdbid);

    if recipeId != -1 {
        this.m_knownRecipes[recipeId] = recipe;
        return;
    }

    ArrayPush(this.m_knownRecipes, recipe);
    ArrayPush(this.m_newRecipes, targetTdbid);
}