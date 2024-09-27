#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>

#include <inventory/inventoryNode.hpp>
#include "../resultContext.hpp"

namespace InventoryReader
{
class NGPlusItemData : public Red::ISerializable
{
public:
    Red::ItemID m_itemId{};
    int m_itemQuantity{};

    Red::DynArray<Red::ItemID> m_attachments;
    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> m_statModifiers;

    RTTI_IMPL_TYPEINFO(NGPlusItemData);
    RTTI_IMPL_ALLOCATOR();
};

struct InventoryReaderResults
{
    Red::DynArray<Red::Handle<NGPlusItemData>> m_itemDataInventory;
    Red::DynArray<Red::Handle<NGPlusItemData>> m_itemDataStash;

    int m_playerMoney{};
};

InventoryReaderResults GetData(save::InventoryNode& aInventory, ResultContext& aContext) noexcept;
}

RTTI_DEFINE_CLASS(InventoryReader::NGPlusItemData, {
    RTTI_GETTER(m_itemId);
    RTTI_GETTER(m_itemQuantity);
    RTTI_GETTER(m_attachments);
    RTTI_GETTER(m_statModifiers);
});