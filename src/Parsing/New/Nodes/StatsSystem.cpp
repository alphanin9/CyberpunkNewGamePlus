#include <Shared/Util/NamePoolRegistrar.hpp>
#include "StatsSystem.hpp"

using namespace shared::raw;
using namespace shared::util;

using namespace parser::node;
using namespace Red;

bool StatsSystemNode::OnRead(Save::Stream::LoadStream& aStream) noexcept
{
    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (!aStream.IsGood())
    {
        return false;
    }

    m_handle = aStream.ReadPackage(GetClass<game::StatsStateMapStructure>());

    if (!m_handle)
    {
        return false;
    }

    m_statsStruct = reinterpret_cast<game::StatsStateMapStructure*>(m_handle.instance);
    m_idToStatsMap.reserve(m_statsStruct->keys.size);

    for (auto i = 0u; i < m_statsStruct->keys.size; i++)
    {
        m_idToStatsMap.insert_or_assign(m_statsStruct->keys[i].entityHash, &m_statsStruct->values[i]);
    }

    return aStream.IsGood();
}

CName StatsSystemNode::GetName() noexcept
{
    return NamePoolRegistrar<"ScriptableSystemsContainer">::Get();
}

RTTI_DEFINE_CLASS(parser::node::StatsSystemNode, { RTTI_PARENT(parser::node::SaveNodeData); });