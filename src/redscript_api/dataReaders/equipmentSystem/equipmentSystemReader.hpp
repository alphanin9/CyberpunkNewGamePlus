#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../resultContext.hpp"

namespace EquipmentSystemReader
{
struct EquipmentSystemResults
{
    Red::ItemID m_playerEquippedOperatingSystem{};
    Red::ItemID m_playerEquippedKiroshis{};
    Red::ItemID m_playerEquippedLegCyberware{};
    Red::ItemID m_playerEquippedArmCyberware{};

    Red::DynArray<Red::ItemID> m_playerEquippedCardiacSystemCW;
};

EquipmentSystemResults GetData(Red::Handle<Red::ISerializable>* aEquipmentSystem, ResultContext& aContext) noexcept;
}