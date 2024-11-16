// EquipmentEx is different from everything else in that it's not guaranteed to be present in save
// Not too different in loading, though

#include "outfitSystemReader.hpp"

#include <Shared/RTTI/PropertyAccessor.hpp>

using namespace Red;

OutfitSystemReader::OutfitSystemResults::OutfitSystemResults(Handle<ISerializable>* aOutfitSystem) noexcept
{
    auto& instance = *aOutfitSystem;
    auto& outfitState = shared::rtti::GetClassProperty<Handle<IScriptable>, "state">(instance);
    auto& outfitSets = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "outfits">(outfitState);

    for (auto& handle : outfitSets)
    {
        auto outfitSet = MakeHandle<NGPlusOutfitSet>();

        outfitSet->m_name = shared::rtti::GetClassProperty<CName, "name">(handle);

        auto& outfitSetParts = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "parts">(handle);

        for (auto& part : outfitSetParts)
        {
            auto outfitPart = MakeHandle<NGPlusOutfitPart>();

            outfitPart->m_itemID = shared::rtti::GetClassProperty<ItemID, "itemID">(part);
            outfitPart->m_slotID = shared::rtti::GetClassProperty<TweakDBID, "slotID">(part);

            outfitSet->m_outfitParts.PushBack(std::move(outfitPart));
        }

        this->m_data.PushBack(std::move(outfitSet));
    }
}