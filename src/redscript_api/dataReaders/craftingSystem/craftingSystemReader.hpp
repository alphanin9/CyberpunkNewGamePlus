#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace CraftingSystemReader
{
class NGPlusCraftingInfo : public Red::ISerializable
{
public:
    Red::TweakDBID m_targetItem;
    int m_amount{};
    Red::DynArray<Red::ItemID> m_hideOnItemsAdded;

    Red::TweakDBID GetTargetItem()
    {
        return m_targetItem;
    }

    int GetAmount()
    {
        return m_amount;
    }

    Red::DynArray<Red::ItemID> GetHideOnItemsAdded()
    {
        return m_hideOnItemsAdded;
    }

    RTTI_IMPL_TYPEINFO(NGPlusCraftingInfo);
    RTTI_IMPL_ALLOCATOR();
};

struct CraftingSystemResults
{
    Red::DynArray<Red::Handle<NGPlusCraftingInfo>> m_data;
};

CraftingSystemResults GetData(Red::Handle<Red::ISerializable>* aCraftingSystem) noexcept;
}

RTTI_DEFINE_CLASS(CraftingSystemReader::NGPlusCraftingInfo, {
    RTTI_GETTER(m_targetItem);
    RTTI_GETTER(m_amount);
    RTTI_GETTER(m_hideOnItemsAdded);
                                                            });