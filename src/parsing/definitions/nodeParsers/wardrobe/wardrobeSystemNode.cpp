#include "WardrobeSystemNode.hpp"
#include <Parsing/Definitions/NodeParsers/Inventory/InventoryNode.hpp>

using namespace Red;

modsave::WardrobeEntry::WardrobeEntry(FileCursor& aCursor)
{
    m_appearanceName = aCursor.OptimizedReadLengthPrefixedCName();
    m_itemId = item::ReadItemId(aCursor);
}

// Note: wardrobe items still seem to carry quality with them from stats
void modsave::WardrobeSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto itemCount = aCursor.readInt();

    m_wardrobeEntries.reserve(itemCount);

    for (auto i = 0; i < itemCount; i++)
    {
        m_wardrobeEntries.emplace_back(aCursor);
    }
}

const std::vector<modsave::WardrobeEntry>& modsave::WardrobeSystemNode::GetWardrobe() noexcept
{
    return m_wardrobeEntries;
}