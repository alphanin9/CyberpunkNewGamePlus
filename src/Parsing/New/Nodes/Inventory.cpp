#include <Shared/Util/NamePoolRegistrar.hpp>

#include "Inventory.hpp"

using namespace parser::node;
using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool InventoryNode::OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept
{
    static const auto c_itemDataNodeName = NamePoolRegistrar<"itemData">::Get();

    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (!aStream.IsGood())
    {
        return false;
    }

    // First field inside the "inventory" node is the per-owner bucket count.
    // The game reads it via the raw stream read primitive (stream vtable +0x10, size 4 -> sub_14024B3CC).
    m_ownerBucketCount = 0u;
    aStream->ReadWriteEx(&m_ownerBucketCount);

    // TODO(issue #4, Phase 2): per-owner / per-item reconstruction.
    //
    // Validated read sequence from game::InventoryManager::OnGameLoad (0x14099C388) on 2.31:
    //   for each of m_ownerBucketCount owner buckets:
    //     ownerHash = read_u64   (stream +0x10, size 8 -> sub_140692DDC)
    //     itemCount = read_u32   (stream +0x10, size 4 -> sub_14024B3CC)
    //     if itemCount == 0: continue
    //     for each item:
    //       read 16-byte outer item header (record ID + class-selection bytes, version gated 0x61/0xBE/0xDD)
    //       resolve the item record; validate record type against the header
    //       select concrete item-data class (StackedItemData 0x90 / UniqueItemData 0x100 / BlueprintStackableItemData 0x108)
    //       open the "itemData" child node and deserialize:
    //         - item-local fields
    //         - top-level StatsObjectID (item + 0x9C, 16 bytes incl. the +0xC aux dword) via game_StatsObjectID_Serialize (0x14099C258)
    //         - recursive InnerItemData tree (item + 0xB0) via game_InnerItemData_Serialize (0x14099C098); each node carries its own StatsObjectID
    //       skip the hardcoded "Items.Minotaur_HMG_Left" special case
    //
    // NOTE: for the offline reader we only need the serialized bytes (IDs / quantities / StatsObjectIDs),
    // not the live stat-provider rebinding (game_ItemData_RebindLoadedStatsObject) or the stats replay pass,
    // which only matter when loading into a live session. See inventorymanager_ongameload_re_note.md.
    // Reference implementation of the byte grammar: the custom modsave::InventoryNode parser.

    return aStream.IsGood();
}

CName InventoryNode::GetName() noexcept
{
    return NamePoolRegistrar<"inventory">::Get();
}

RTTI_DEFINE_CLASS(parser::node::InventoryNode, { RTTI_PARENT(parser::node::SaveNodeData); });