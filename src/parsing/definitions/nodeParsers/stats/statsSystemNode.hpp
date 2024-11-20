#pragma once
#include "../interfaceNodeData.hpp"
#include "../../nodeEntry.hpp"
#include "../../package/packageReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/SavedStatsData.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/StatModifierData_Deprecated.hpp>

namespace modsave
{
class StatsSystemNode : public NodeDataInterface
{
    package::Package m_package;
    std::unordered_map<std::uint64_t, Red::game::SavedStatsData*> m_statsMap;
public:
    static constexpr Red::CName m_nodeName = "StatsSystem";

    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;

    static std::uint64_t GetEntityHashFromItemId(const Red::ItemID& aId) noexcept;

    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetStatModifiers(
        std::uint64_t aEntityHash) noexcept;

    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetForcedModifiers(
        std::uint64_t aEntityHash) noexcept;

    Red::DynArray<Red::game::data::StatType> GetDisabledModifiers(std::uint64_t aEntityHash) noexcept;

    static void DumpStatModifiersToConsole(
        const Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>>& aStatModifierList) noexcept;
};
}