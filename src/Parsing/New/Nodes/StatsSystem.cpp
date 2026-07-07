#include <Shared/Raw/StatsSystem/StatsSystem.hpp>
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

game::SavedStatsData* StatsSystemNode::GetSavedStatsData(std::uint64_t aEntityHash) noexcept
{
    const auto it = m_idToStatsMap.find(aEntityHash);

    if (it == m_idToStatsMap.end())
    {
        return nullptr;
    }

    return it->second;
}

DynArray<Handle<ISerializable>> StatsSystemNode::GetStatModifiers(std::uint64_t aEntityHash) noexcept
{
    auto* data = GetSavedStatsData(aEntityHash);

    if (!data)
    {
        return {};
    }

    return shared::raw::StatsSystem::ReadSavedModifiers(data->modifiersBuffer);
}

DynArray<Handle<ISerializable>> StatsSystemNode::GetForcedModifiers(std::uint64_t aEntityHash) noexcept
{
    auto* data = GetSavedStatsData(aEntityHash);

    if (!data)
    {
        return {};
    }

    return shared::raw::StatsSystem::ReadSavedModifiers(data->forcedModifiersBuffer);
}

CName StatsSystemNode::GetName() noexcept
{
    // The game opens the "StatsSystem" save node (GetStatsSystemSaveNodeName @ 0x1418C44E8,
    // verified against game::StatsSystem::OnGameLoad on 2.31). The previous "ScriptableSystemsContainer"
    // literal was a copy-paste from ScriptableSystemsContainerNode and seeked the wrong node.
    return NamePoolRegistrar<"StatsSystem">::Get();
}

RTTI_DEFINE_CLASS(parser::node::StatsSystemNode, { RTTI_PARENT(parser::node::SaveNodeData); });