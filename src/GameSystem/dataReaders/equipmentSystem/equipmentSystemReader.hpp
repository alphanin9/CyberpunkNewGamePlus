#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../resultContext.hpp"

namespace EquipmentSystemReader
{
class EquipmentSystemResults : public Red::IScriptable
{
public:
    Red::ItemID m_playerEquippedOperatingSystem{};
    Red::ItemID m_playerEquippedKiroshis{};
    Red::ItemID m_playerEquippedLegCyberware{};
    Red::ItemID m_playerEquippedArmCyberware{};

    Red::DynArray<Red::ItemID> m_playerEquippedCardiacSystemCW;

    EquipmentSystemResults() = default;
    EquipmentSystemResults(Red::Handle<Red::ISerializable>* aEquipmentSystem, ResultContext& aContext) noexcept;

    RTTI_IMPL_ALLOCATOR();
    RTTI_IMPL_TYPEINFO(EquipmentSystemResults);
};
}

RTTI_DEFINE_CLASS(EquipmentSystemReader::EquipmentSystemResults, {
    RTTI_GETTER(m_playerEquippedOperatingSystem);
    RTTI_GETTER(m_playerEquippedKiroshis);
    RTTI_GETTER(m_playerEquippedLegCyberware);
    RTTI_GETTER(m_playerEquippedArmCyberware);
    RTTI_GETTER(m_playerEquippedCardiacSystemCW);
});