#include "equipmentSystemReader.hpp"

#include "scriptable/helpers/classDefinitions/equipmentSystem.hpp"

using namespace Red;

using scriptable::native::EquipmentSystem::EquipmentSystemPlayerData;

namespace EquipmentSystemReader
{
EquipmentSystemPlayerData GetEquipmentSystemPlayerData(Red::Handle<Red::ISerializable>* aEquipmentSystem)
{
    auto& instance = *aEquipmentSystem;

    auto prop = instance->GetType()->GetProperty("ownerData");

    auto arrayInstance = prop->GetValuePtr<void>(instance);

    return reinterpret_cast<CRTTIBaseArrayType*>(prop->type)->GetElement(arrayInstance, 0u);
}

EquipmentSystemResults GetData(Red::Handle<Red::ISerializable>* aEquipmentSystem, ResultContext& aContext) noexcept
{
    EquipmentSystemResults result{};

    auto data = GetEquipmentSystemPlayerData(aEquipmentSystem);

    if (!data)
    {
        return result;
    }

    auto loadout = data.GetLoadout();

    using game::data::EquipmentArea;

    for (auto& area : loadout->equipAreas)
    {
        switch (area.areaType)
        {
        case EquipmentArea::EyesCW:
        {
            if (result.m_playerEquippedKiroshis.IsValid())
            {
                break;
            }

            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;
                if (!itemId.IsValid())
                {
                    continue;
                }

                TweakDBID tagsFlat(itemId.tdbid, ".tags");
                auto flat = aContext.m_tweakDB->GetFlatValue(tagsFlat);

                if (!flat)
                {
                    continue;
                }

                auto tagList = flat->GetValue<DynArray<CName>>();

                if (!tagList->Contains("MaskCW"))
                {
                    result.m_playerEquippedKiroshis = itemId;
                    break;
                }
            }

            break;
        }
        case EquipmentArea::SystemReplacementCW:
        {
            if (result.m_playerEquippedOperatingSystem.IsValid())
            {
                break;
            }

            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (!itemId.IsValid())
                {
                    continue;
                }

                result.m_playerEquippedOperatingSystem = itemId;
                break;
            }
            break;
        case EquipmentArea::CardiovascularSystemCW:
            if (result.m_playerEquippedCardiacSystemCW.size > 0)
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (!itemId.IsValid())
                {
                    continue;
                }

                result.m_playerEquippedCardiacSystemCW.PushBack(itemId);
            }
            break;
        case EquipmentArea::ArmsCW:
            if (result.m_playerEquippedArmCyberware.IsValid())
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (!itemId.IsValid())
                {
                    continue;
                }

                result.m_playerEquippedArmCyberware = itemId;
                break;
            }
            break;
        case EquipmentArea::LegsCW:
            if (result.m_playerEquippedLegCyberware.IsValid())
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (!itemId.IsValid())
                {
                    continue;
                }

                result.m_playerEquippedLegCyberware = itemId;
                break;
            }
            break;
        }
        }
    }

    return result;
}
}