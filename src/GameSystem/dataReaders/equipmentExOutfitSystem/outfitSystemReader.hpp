#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace OutfitSystemReader
{
class NGPlusOutfitPart : public Red::IScriptable
{
public:
    Red::TweakDBID m_slotID;
    Red::ItemID m_itemID;

    RTTI_IMPL_TYPEINFO(NGPlusOutfitPart);
    RTTI_IMPL_ALLOCATOR();
};

class NGPlusOutfitSet : public Red::IScriptable
{
public:
    // No point keeping timestamp, I feel
    Red::DynArray<Red::Handle<NGPlusOutfitPart>> m_outfitParts;
    Red::CName m_name;

    RTTI_IMPL_TYPEINFO(NGPlusOutfitSet);
    RTTI_IMPL_ALLOCATOR();
};

class OutfitSystemResults : public Red::IScriptable
{
public:
    Red::DynArray<Red::Handle<NGPlusOutfitSet>> m_data;
    
    OutfitSystemResults() = default;
    OutfitSystemResults(Red::Handle<Red::ISerializable>* aOutfitSystem) noexcept;

    RTTI_IMPL_TYPEINFO(OutfitSystemResults);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace OutfitSystemReader

RTTI_DEFINE_CLASS(OutfitSystemReader::NGPlusOutfitPart, {
    RTTI_GETTER(m_slotID);
    RTTI_GETTER(m_itemID);
});

RTTI_DEFINE_CLASS(OutfitSystemReader::NGPlusOutfitSet, {
    RTTI_GETTER(m_outfitParts);
    RTTI_GETTER(m_name);
});

RTTI_DEFINE_CLASS(OutfitSystemReader::OutfitSystemResults, { RTTI_GETTER(m_data); });