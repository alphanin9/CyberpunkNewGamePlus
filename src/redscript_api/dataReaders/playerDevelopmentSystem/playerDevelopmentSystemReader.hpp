#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace PlayerDevelopmentSystemReader
{
struct PlayerDevelopmentSystemResults
{
    int m_playerPerkPoints{};
    int m_playerRelicPoints{};
    int m_playerAttributePoints{};

    int m_playerBodyAttribute{};
    int m_playerReflexAttribute{};
    int m_playerTechAttribute{};
    int m_playerIntelligenceAttribute{};
    int m_playerCoolAttribute{};

    int m_playerBodySkillLevel{};
    int m_playerReflexSkillLevel{};
    int m_playerTechSkillLevel{};
    int m_playerIntelligenceSkillLevel{};
    int m_playerCoolSkillLevel{};

    int m_playerLevel{};
    int m_playerStreetCred{};
};

PlayerDevelopmentSystemResults GetData(Red::Handle<Red::ISerializable>* aPlayerDevelopmentSystem) noexcept;
}