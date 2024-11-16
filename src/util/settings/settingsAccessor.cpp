#include "settingsAccessor.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <Shared/RTTI/PropertyAccessor.hpp>

using namespace Red;

settings::ModConfig settings::GetRandomEncounterSettings() noexcept
{
    auto classInstance = MakeScriptedHandle("NGPlus.UserSettings");

    if (!classInstance)
    {
        return {};
    }

    ModConfig config{};

    config.m_enableRandomEncounters = shared::rtti::GetClassProperty<bool, "enableRandomEncounters">(classInstance);
    config.m_useExteriorDetectionForRandomEncounters =
        shared::rtti::GetClassProperty<bool, "spawnRandomEncountersWhileWalking">(classInstance);

    return config;
}