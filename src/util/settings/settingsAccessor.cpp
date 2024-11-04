#include "settingsAccessor.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

using namespace Red;

settings::ModConfig settings::GetRandomEncounterSettings() noexcept
{
    auto classInstance = MakeScriptedHandle("NGPlus.UserSettings");

    if (!classInstance)
    {
        return {};
    }

    ModConfig config{};

    config.m_enableRandomEncounters = classInstance->GetType()->GetProperty("enableRandomEncounters")->GetValue<bool>(classInstance);
    config.m_useExteriorDetectionForRandomEncounters =
        classInstance->GetType()->GetProperty("spawnRandomEncountersWhileWalking")->GetValue<bool>(classInstance);

    return config;
}