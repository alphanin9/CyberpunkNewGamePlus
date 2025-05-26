#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <parsing/New/TypeDefinitions/NGPlusSaveNode.hpp>

#include <Shared/Raw/Save/Save.hpp>

namespace parser::node
{
struct ItemLootData
{
    Red::TweakDBID m_lootPoolId{};
    std::uint32_t unk08{};
    float m_requiredLevel{};
};

struct InnerItemDataRepresentation
{
    Red::ItemID m_itemID{};
    Red::CName m_appearanceName{};
    Red::TweakDBID m_attachmentSlotID{};

    Red::DynArray<InnerItemDataRepresentation> m_children{};

    std::uint32_t unk30{};

    ItemLootData m_lootData{};
};

struct UniqueItemDataRepresentation : Red::ISerializable
{
    RTTI_IMPL_TYPEINFO(UniqueItemDataRepresentation);
    RTTI_IMPL_ALLOCATOR();
};

struct BlueprintStackableItemDataRepresentation : Red::ISerializable
{
    RTTI_IMPL_TYPEINFO(BlueprintStackableItemDataRepresentation);
    RTTI_IMPL_ALLOCATOR();
};

struct StackedItemDataRepresentation : Red::ISerializable
{
    RTTI_IMPL_TYPEINFO(StackedItemDataRepresentation);
    RTTI_IMPL_ALLOCATOR();
};

struct ItemRepresentation : Red::ISerializable
{
    Red::ItemID m_itemID{};
    
    char m_itemFlags{}; // quest, ETC

    RTTI_IMPL_TYPEINFO(ItemRepresentation);
    RTTI_IMPL_ALLOCATOR();
};

class InventoryNode : public SaveNodeData
{
private:
    
public:
    bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept override;
    Red::CName GetName() noexcept override;

    RTTI_IMPL_TYPEINFO(InventoryNode);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace parser::node