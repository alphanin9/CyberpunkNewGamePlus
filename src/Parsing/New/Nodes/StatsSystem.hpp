#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <tsl/hopscotch_map.h>

#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>
#include <parsing/New/TypeDefinitions/NGPlusSaveNode.hpp>

#include <Shared/Raw/Save/Save.hpp>

namespace parser::node
{
class StatsSystemNode : public SaveNodeData
{
private:
    Red::Handle<Red::ISerializable> m_handle{};
    Red::game::StatsStateMapStructure* m_statsStruct{};

    tsl::hopscotch_map<std::uint64_t, Red::game::SavedStatsData*> m_idToStatsMap{};

public:
    bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept override;
    Red::CName GetName() noexcept override;

    // Returns the saved stats data (modifier buffers, inactive stats, recordID, seed) for a given
    // StatsObjectID entityHash, or nullptr if absent. Entity hash 1 is the player.
    // Mirrors the transient StatsObjectID -> SavedStatsData map the game rebuilds in
    // game::StatsSystem::OnGameLoad (saveVersion >= 0xA5 path).
    Red::game::SavedStatsData* GetSavedStatsData(std::uint64_t aEntityHash) noexcept;

    // Deserializes the saved / forced stat modifiers for an entity by replaying the game's own
    // per-modifier serializer over the SavedStatsData buffers. Cast each handle to the concrete
    // gameStatModifierData_Deprecated subclass (Constant/Combined/Curve) as needed.
    Red::DynArray<Red::Handle<Red::ISerializable>> GetStatModifiers(std::uint64_t aEntityHash) noexcept;
    Red::DynArray<Red::Handle<Red::ISerializable>> GetForcedModifiers(std::uint64_t aEntityHash) noexcept;

    RTTI_IMPL_TYPEINFO(StatsSystemNode);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace parser::node