// EquipmentEx is different from everything else in that it's not guaranteed to be present in save
// Not too different in loading, though

#include "outfitSystemReader.hpp"

using namespace Red;
namespace OutfitSystemReader
{
OutfitSystemResults GetData(Handle<ISerializable>* aOutfitSystem) noexcept
{
    auto& instance = *aOutfitSystem;
    auto& outfitState =
        *instance->GetType()->GetProperty("state")->GetValuePtr<Red::Handle<Red::IScriptable>>(instance);

    auto& outfitSets =
        *outfitState->GetType()->GetProperty("outfits")->GetValuePtr<DynArray<Handle<IScriptable>>>(outfitState);

    OutfitSystemResults result{};

    for (auto& handle : outfitSets)
    {
        auto outfitSet = MakeHandle<NGPlusOutfitSet>();

        outfitSet->m_name = handle->GetType()->GetProperty("name")->GetValue<CName>(handle);

        auto& outfitSetParts =
            *handle->GetType()->GetProperty("parts")->GetValuePtr<DynArray<Handle<IScriptable>>>(handle);

        for (auto& part : outfitSetParts)
        {
            auto outfitPart = MakeHandle<NGPlusOutfitPart>();

            outfitPart->m_itemID = part->GetType()->GetProperty("itemID")->GetValue<ItemID>(part);
            outfitPart->m_slotID = part->GetType()->GetProperty("slotID")->GetValue<TweakDBID>(part);

            outfitSet->m_outfitParts.PushBack(std::move(outfitPart));
        }

        result.m_data.PushBack(std::move(outfitSet));
    }

    return result;
}
} // namespace OutfitSystemReader
