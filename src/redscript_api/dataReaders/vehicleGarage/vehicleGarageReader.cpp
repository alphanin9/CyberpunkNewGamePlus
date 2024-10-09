#include "vehicleGarageReader.hpp"

using namespace Red;

VehicleGarageReader::VehicleGarageResults::VehicleGarageResults(const Handle<GarageComponentPS>& aGarage) noexcept
{
    constexpr TweakDBID Demiurge = "Vehicle.v_utility4_thorton_mackinaw_bmf_player";
    constexpr TweakDBID Hoon = "Vehicle.v_sport2_quadra_type66_nomad_tribute";
    constexpr TweakDBID JackieArch = "Vehicle.v_sportbike2_arch_jackie_player";
    constexpr TweakDBID JackieTunedArch = "Vehicle.v_sportbike2_arch_jackie_tuned_player";

    for (auto& i : aGarage->unlockedVehicleArray)
    {
        auto tdbid = i.vehicleID.recordID;

        if (tdbid != Demiurge && tdbid != Hoon && tdbid != JackieArch && tdbid != JackieTunedArch)
        {
            m_garage.PushBack(tdbid);
        }
    }
}