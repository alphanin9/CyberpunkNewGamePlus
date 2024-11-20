#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>

#include "../resultContext.hpp"
#include <parsing/definitions/nodeParsers/inventory/inventoryNode.hpp>

namespace InventoryReader
{
class NGPlusItemData : public Red::IScriptable
{
public:
    Red::ItemID m_itemId{};
    int m_itemQuantity{};

    Red::DynArray<Red::ItemID> m_attachments;
    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> m_statModifiers;

    RTTI_IMPL_TYPEINFO(NGPlusItemData);
    RTTI_IMPL_ALLOCATOR();
};

class InventoryReaderResults : public Red::IScriptable
{
public:
    Red::DynArray<Red::Handle<NGPlusItemData>> m_inventory;
    Red::DynArray<Red::Handle<NGPlusItemData>> m_stash;

    int m_money{};

    InventoryReaderResults() = default;
    InventoryReaderResults(modsave::InventoryNode& aInventory, ResultContext& aContext) noexcept;

    RTTI_IMPL_TYPEINFO(InventoryReaderResults);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace InventoryReader

RTTI_DEFINE_CLASS(InventoryReader::NGPlusItemData, {
    RTTI_GETTER(m_itemId);
    RTTI_GETTER(m_itemQuantity);
    RTTI_GETTER(m_attachments);
    RTTI_GETTER(m_statModifiers);
});

RTTI_DEFINE_CLASS(InventoryReader::InventoryReaderResults, {
    RTTI_GETTER(m_inventory);
    RTTI_GETTER(m_stash);
    RTTI_GETTER(m_money);
});