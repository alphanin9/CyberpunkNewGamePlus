#include <format>

#include "factsDBNode.hpp"

#include "../../nodeEntry.hpp"
#include "../parserHelper.hpp"

#include <context.hpp>

namespace save
{
void FactsTableNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto factCount = aCursor.readVlqInt32();

    m_keys = aCursor.ReadSpan<std::uint32_t>(factCount);
    m_values = aCursor.ReadSpan<std::uint32_t>(factCount);
}

void FactsDBNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    m_tableCount = std::min(std::size_t(aCursor.readValue<std::uint8_t>()), std::size_t(11));

    for (auto i = 0u; i < aNode.nodeChildren.size(); i++)
    {
        auto child = aNode.nodeChildren[i];

        ParseNode(aCursor, *child);

        child->isReadByParent = true;

        m_factsTables.push_back(static_cast<FactsTableNode*>(child->nodeData.get()));
    }

    // Apparently some trailing stuff gets thrown in...
    aCursor.seekTo(aNode.offset + aNode.GetExpectedSize());

    constexpr auto testReader = false;

    if constexpr (testReader) 
    {
        PluginContext::Spew(std::format("NG+ active: {}", GetFact("ngplus_active")));
        PluginContext::Spew(std::format("Q101 done: {}", GetFact("q101_done")));
    }
}

// Not very efficient, might get slow...
// Could put all facts tables into one big unordered map?
std::uint32_t FactsDBNode::GetFact(const char* aName) noexcept
{
    return GetFact(Red::FNV1a32(aName));
}

std::uint32_t FactsDBNode::GetFact(std::uint32_t aHash) noexcept
{
    for (auto tablePtr : m_factsTables)
    {
        for (auto i = 0u; i < tablePtr->m_keys.size(); i++)
        {
            if (aHash == tablePtr->m_keys[i])
            {
                return tablePtr->m_values[i];
            }
        }
    }

    return 0u;
}
}
