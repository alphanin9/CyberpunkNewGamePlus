#include "wardrobeSystemNode.hpp"
#include "../inventory/inventoryNode.hpp"

using namespace Red;

save::WardrobeEntry::WardrobeEntry(FileCursor& aCursor)
{
    m_appearanceName = aCursor.OptimizedReadLengthPrefixedCName();
    m_itemId = item::ReadItemId(aCursor);
}

// Note: wardrobe items still seem to carry quality with them from stats
void save::WardrobeSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto itemCount = aCursor.readInt();

    m_wardrobeEntries.reserve(itemCount);

    for (auto i = 0; i < itemCount; i++)
    {
        m_wardrobeEntries.emplace_back(aCursor);
    }
}

const std::vector<save::WardrobeEntry>& save::WardrobeSystemNode::GetWardrobe() noexcept
{
    return m_wardrobeEntries;
}