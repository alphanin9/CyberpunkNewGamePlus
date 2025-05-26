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

    RTTI_IMPL_TYPEINFO(StatsSystemNode);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace parser::node