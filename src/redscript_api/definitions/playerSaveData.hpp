#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../dataReaders/craftingSystem/craftingSystemReader.hpp"
#include "../dataReaders/equipmentExOutfitSystem/outfitSystemReader.hpp"
#include "../dataReaders/equipmentExViewManager/viewManagerReader.hpp"
#include "../dataReaders/equipmentSystem/equipmentSystemReader.hpp"
#include "../dataReaders/inventory/inventoryReader.hpp"
#include "../dataReaders/playerDevelopmentSystem/playerDevelopmentSystemReader.hpp"
#include "../dataReaders/statsSystem/statsSystemReader.hpp"
#include "../dataReaders/vehicleGarage/vehicleGarageReader.hpp"
#include "../dataReaders/wardrobeSystem/wardrobeReader.hpp"

#include "../../parsing/fileReader.hpp"

class NGPlusProgressionData : public Red::IScriptable
{
public:
    Red::Handle<CraftingSystemReader::CraftingSystemResults> m_craftingSystemResults{};
    Red::Handle<OutfitSystemReader::OutfitSystemResults> m_equipmentExOutfitSystemResults{};
    Red::Handle<ViewManagerReader::ViewManagerResults> m_equipmentExViewManagerResults{};
    Red::Handle<EquipmentSystemReader::EquipmentSystemResults> m_equipmentSystemResults{};
    Red::Handle<InventoryReader::InventoryReaderResults> m_playerInventory{};
    Red::Handle<PlayerDevelopmentSystemReader::PlayerDevelopmentSystemResults> m_playerDevelopmentSystemResults{};
    Red::Handle<StatsSystemReader::StatsSystemResults> m_statsSystemResults{};
    Red::Handle<VehicleGarageReader::VehicleGarageResults> m_vehicleGarageResults{};
    Red::Handle<WardrobeReader::WardrobeResults> m_wardrobeResults{};
    
    NGPlusProgressionData() = default;
    NGPlusProgressionData(parser::Parser& aParser) noexcept;
    void PostProcess() noexcept;

    RTTI_IMPL_TYPEINFO(NGPlusProgressionData);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(NGPlusProgressionData, {
    RTTI_GETTER(m_craftingSystemResults);
    RTTI_GETTER(m_equipmentExOutfitSystemResults);
    RTTI_GETTER(m_equipmentExViewManagerResults);
    RTTI_GETTER(m_equipmentSystemResults);
    RTTI_GETTER(m_playerInventory);
    RTTI_GETTER(m_playerDevelopmentSystemResults);
    RTTI_GETTER(m_statsSystemResults);
    RTTI_GETTER(m_vehicleGarageResults);
    RTTI_GETTER(m_wardrobeResults);
});