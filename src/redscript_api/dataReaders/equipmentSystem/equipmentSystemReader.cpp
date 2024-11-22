#include "equipmentSystemReader.hpp"

#include <parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/equipmentSystem.hpp>

#include <Shared/RTTI/PropertyAccessor.hpp>

using namespace Red;

using scriptable::native::EquipmentSystem::EquipmentSystemPlayerData;

EquipmentSystemReader::EquipmentSystemResults::EquipmentSystemResults(Handle<ISerializable>* aEquipmentSystem,
                                                                      ResultContext& aContext) noexcept
{
    auto& instance = *aEquipmentSystem;

    auto& ownerData = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "ownerData">(instance);
    auto& equipment = shared::rtti::GetClassProperty<SLoadout, "equipment">(ownerData[0]);

    using game::data::EquipmentArea;

    for (auto& area : equipment.equipAreas)
    {
        switch (area.areaType)
        {
        case EquipmentArea::EyesCW:
        {
            if (m_playerEquippedKiroshis)
            {
                break;
            }

            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;
                if (itemId)
                {
                    TweakDBID tagsFlat(itemId.tdbid, ".tags");
                    auto flat = aContext.m_tweakDB->GetFlatValue(tagsFlat);

                    if (!flat)
                    {
                        continue;
                    }

                    auto tagList = flat->GetValue<DynArray<CName>>();

                    if (!tagList->Contains("MaskCW"))
                    {
                        m_playerEquippedKiroshis = itemId;
                        break;
                    }
                }
            }

            break;
        }
        case EquipmentArea::SystemReplacementCW:
        {
            if (m_playerEquippedOperatingSystem)
            {
                break;
            }

            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (itemId)
                {
                    m_playerEquippedOperatingSystem = itemId;
                    break;
                }
            }
            break;
        case EquipmentArea::CardiovascularSystemCW:
            if (m_playerEquippedCardiacSystemCW.size > 0)
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (itemId)
                {
                    m_playerEquippedCardiacSystemCW.PushBack(itemId);
                }
            }
            break;
        case EquipmentArea::ArmsCW:
            if (m_playerEquippedArmCyberware)
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (itemId)
                {
                    m_playerEquippedArmCyberware = itemId;
                    break;
                }
            }
            break;
        case EquipmentArea::LegsCW:
            if (m_playerEquippedLegCyberware)
            {
                break;
            }
            for (auto& equippedItemData : area.equipSlots)
            {
                auto& itemId = equippedItemData.itemID;

                if (itemId)
                {
                    m_playerEquippedLegCyberware = itemId;
                    break;
                }
            }
            break;
        }
        }
    }
}