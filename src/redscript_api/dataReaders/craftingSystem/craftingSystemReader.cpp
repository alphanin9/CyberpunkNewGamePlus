#include "craftingSystemReader.hpp"

#include <parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/craftBook.hpp>
#include <Shared/RTTI/PropertyAccessor.hpp>

using namespace Red;

using scriptable::native::CraftBook::CraftBook;
using scriptable::native::CraftBook::ItemRecipe;

CraftingSystemReader::CraftingSystemResults::CraftingSystemResults(Handle<ISerializable>* aCraftingSystem) noexcept
{
    auto& instance = *aCraftingSystem;

    CraftBook craftBook = shared::rtti::GetClassProperty<Handle<IScriptable>, "playerCraftBook">(*aCraftingSystem).instance;

    if (!craftBook)
    {
        return;
    }

    craftBook.IterateOverRecipes(
        [this](ItemRecipe recipe)
        {
            auto craftingInfo = MakeHandle<NGPlusCraftingInfo>();

            craftingInfo->m_amount = recipe.GetAmount();
            craftingInfo->m_targetItem = recipe.GetTargetItem();
            craftingInfo->m_hideOnItemsAdded = recipe.GetHideOnAdded();

            this->m_data.PushBack(std::move(craftingInfo));
        });
}