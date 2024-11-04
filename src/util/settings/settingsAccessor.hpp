#pragma once

namespace settings
{
struct ModConfig
{
    bool m_enableRandomEncounters{};
    bool m_useExteriorDetectionForRandomEncounters{};
};
ModConfig GetRandomEncounterSettings() noexcept;
} // namespace settings