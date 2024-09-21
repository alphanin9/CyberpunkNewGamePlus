#include "wardrobeSystemNode.hpp"
#include "../inventory/inventoryNode.hpp"

#include <context.hpp>

using namespace Red;

save::WardrobeEntry::WardrobeEntry(FileCursor& aCursor)
{
    m_appearanceName = aCursor.OptimizedReadLengthPrefixedCName();
    m_itemId = item::ReadItemId(aCursor);
}

void save::WardrobeSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto itemCount = aCursor.readInt();

    m_wardrobeEntries.reserve(itemCount);

    for (auto i = 0; i < itemCount; i++)
    {
        m_wardrobeEntries.emplace_back(aCursor);
    }

    constexpr auto spewWardrobeCount = false;
    if constexpr (spewWardrobeCount)
    {
        PluginContext::Spew("Wardrobe store count: {}", m_wardrobeEntries.size());
    }
}

const std::vector<save::WardrobeEntry>& save::WardrobeSystemNode::GetWardrobe() noexcept
{
    return m_wardrobeEntries;
}