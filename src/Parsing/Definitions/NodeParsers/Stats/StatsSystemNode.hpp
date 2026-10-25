#pragma once
#include <Parsing/Definitions/NodeEntry.hpp>
#include <Parsing/Definitions/NodeParsers/InterfaceNodeData.hpp>
#include <Parsing/Definitions/Package/PackageReader.hpp>
#include <deque>

#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/SavedStatsData.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/SavedStatsDataOptimized.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/BufferSlice.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/StatModifierData_Deprecated.hpp>

namespace modsave
{
class StatsSystemNode : public NodeDataInterface
{
    package::Package m_package;

    // Save version 0x114+ (patch 2.5): per-object stats are rebuilt from the shared pools as
    // plain SDK structs; a deque keeps the pointers stored in m_statsMap stable.
    std::deque<Red::game::SavedStatsData> m_rebuiltStatsData;
    std::unordered_map<std::uint64_t, Red::game::SavedStatsData*> m_statsMap;
public:
    static constexpr Red::CName m_nodeName = "StatsSystem";

    // StatsSystem::OnGameLoad switches from keys/values to the shared-pool format at this
    // save version (the game's own gate; 2.5 writes 0x115).
    static constexpr std::uint32_t m_optimizedStatsFormatSaveVersion = 0x114;

    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;

    static std::uint64_t GetEntityHashFromItemId(const Red::ItemID& aId) noexcept;

    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetStatModifiers(
        std::uint64_t aEntityHash) noexcept;

    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetForcedModifiers(
        std::uint64_t aEntityHash) noexcept;

    Red::DynArray<Red::game::data::StatType> GetDisabledModifiers(std::uint64_t aEntityHash) noexcept;

    static void DumpStatModifiersToConsole(
        const Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>>& aStatModifierList) noexcept;

private:
    void ReadOptimizedStatsData(Red::game::StatsStateMapStructure& aStatsStateMap) noexcept;
};
}