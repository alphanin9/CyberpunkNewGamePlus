#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

namespace VehicleGarageReader
{
class VehicleGarageResults : public Red::IScriptable
{
public:
    Red::DynArray<Red::TweakDBID> m_garage{};

    VehicleGarageResults() = default;
    VehicleGarageResults(const Red::Handle<Red::GarageComponentPS>& aGarage) noexcept;

    RTTI_IMPL_TYPEINFO(VehicleGarageResults);
    RTTI_IMPL_ALLOCATOR();
};
}

RTTI_DEFINE_CLASS(VehicleGarageReader::VehicleGarageResults, { RTTI_GETTER(m_garage); });