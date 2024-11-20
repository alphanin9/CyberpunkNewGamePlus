#include "wardrobeReader.hpp"

using namespace Red;

WardrobeReader::WardrobeResults::WardrobeResults(modsave::WardrobeSystemNode& aNode) noexcept
{
    auto& entries = aNode.GetWardrobe();

    m_wardrobe.Reserve(static_cast<std::uint32_t>(entries.size()));

    for (auto& entry : entries)
    {
        m_wardrobe.PushBack(entry.m_itemId);
    }
}