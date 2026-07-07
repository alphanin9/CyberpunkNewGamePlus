#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include <parsing/New/TypeDefinitions/NGPlusSaveNode.hpp>

#include <Shared/Raw/Save/Save.hpp>

namespace parser::node
{
// Reads the modern "PersistencySystem2" save node (saveVersion >= 205) and deserializes the
// vehicleGarageComponentPS persistent-state blob, which is all the NG+ transfer needs from persistency.
//
// The node is a slot stream: each occupied slot carries a persistencyId, a class CName and a versioned
// class-body blob. We reuse the game's own class-body deserializer (shared::raw::Persistency) for the
// single slot we care about and skip the rest by size. Validated against
// game::PersistencySystem::OnGameLoad (0x140249E80) on 2.31.
class PersistencySystemNode : public SaveNodeData
{
private:
    Red::Handle<Red::GarageComponentPS> m_vehicleGarage{};

public:
    bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept override;
    Red::CName GetName() noexcept override;

    const Red::Handle<Red::GarageComponentPS>& GetVehicleGarage() const noexcept
    {
        return m_vehicleGarage;
    }

    RTTI_IMPL_TYPEINFO(PersistencySystemNode);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace parser::node
