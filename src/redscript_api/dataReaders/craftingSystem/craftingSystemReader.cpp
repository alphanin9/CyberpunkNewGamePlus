#include "craftingSystemReader.hpp"

#include <scriptable/helpers/classDefinitions/craftBook.hpp>

using namespace Red;

using scriptable::native::CraftBook::CraftBook;
using scriptable::native::CraftBook::ItemRecipe;

namespace CraftingSystemReader
{
CraftingSystemResults GetData(Handle<ISerializable>* aCraftingSystem) noexcept
{
    CraftingSystemResults result{};

    auto& instance = *aCraftingSystem;

    CraftBook craftBook = instance->GetType()->GetProperty("playerCraftBook")->GetValuePtr<void>(instance);

    if (!craftBook)
    {
        return result;
    }

    craftBook.IterateOverRecipes(
        [&result](ItemRecipe recipe)
        {
            auto craftingInfo = MakeHandle<NGPlusCraftingInfo>();

            craftingInfo->m_amount = recipe.GetAmount();
            craftingInfo->m_targetItem = recipe.GetTargetItem();
            craftingInfo->m_hideOnItemsAdded = recipe.GetHideOnAdded();

            result.m_data.PushBack(std::move(craftingInfo));
        });

    return result;
}
}   