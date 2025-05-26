#pragma once

namespace settings
{
struct ModConfig
{
    bool m_enableRandomEncounters{};
    bool m_useExteriorDetectionForRandomEncounters{};
    bool m_clampPlayerLevel{};
    bool m_disableSaveFileValidation{};
};
ModConfig GetModSettings() noexcept;
} // namespace settings