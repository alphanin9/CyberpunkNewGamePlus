#include "playerSaveData.hpp"
#include "../dataReaders/resultContext.hpp"

#include <parsing/definitions/nodeParsers/persistency/persistencySystemNode.hpp>
#include <parsing/definitions/nodeParsers/scriptable/scriptableContainerNodeV2.hpp>
#include <parsing/definitions/nodeParsers/stats/statsSystemNode.hpp>

using namespace Red;

NGPlusProgressionData::NGPlusProgressionData(parser::Parser& aParser) noexcept
{
    ResultContext ctx{};

	ctx.m_tweakDB = TweakDB::Get();
    ctx.m_statsSystem = aParser.LookupNodeData<modsave::StatsSystemNode>();

    m_statsSystemResults = MakeHandle<StatsSystemReader::StatsSystemResults>(ctx);

	if (auto scriptableSystemsContainer = aParser.LookupNodeData<modsave::ScriptableSystemsContainerNodeV2>())
    {
        constexpr CName c_craftingSystem = "CraftingSystem";
        constexpr CName c_outfitSystem = "EquipmentEx.OutfitSystem";
        constexpr CName c_viewManager = "EquipmentEx.ViewManager";
        constexpr CName c_equipmentSystem = "EquipmentSystem";
        constexpr CName c_playerDevelopmentSystem = "PlayerDevelopmentSystem";

        if (auto craftingSystem = scriptableSystemsContainer->GetScriptableSystem(c_craftingSystem))
        {
            m_craftingSystemResults = MakeHandle<CraftingSystemReader::CraftingSystemResults>(craftingSystem);
        }

        if (auto outfitSystem = scriptableSystemsContainer->GetScriptableSystem(c_outfitSystem))
        {
            m_equipmentExOutfitSystemResults = MakeHandle<OutfitSystemReader::OutfitSystemResults>(outfitSystem);
        }

        if (auto viewManager = scriptableSystemsContainer->GetScriptableSystem(c_viewManager))
        {
            m_equipmentExViewManagerResults = MakeHandle<ViewManagerReader::ViewManagerResults>(viewManager);
        }

        if (auto equipmentSystem = scriptableSystemsContainer->GetScriptableSystem(c_equipmentSystem))
        {
            m_equipmentSystemResults = MakeHandle<EquipmentSystemReader::EquipmentSystemResults>(equipmentSystem, ctx);
        }

        if (auto playerDevelopmentSystem = scriptableSystemsContainer->GetScriptableSystem(c_playerDevelopmentSystem))
        {
            m_playerDevelopmentSystemResults =
                MakeHandle<PlayerDevelopmentSystemReader::PlayerDevelopmentSystemResults>(playerDevelopmentSystem);
        }
    }

    if (auto inventory = aParser.LookupNodeData<modsave::InventoryNode>())
    {
        m_playerInventory = MakeHandle<InventoryReader::InventoryReaderResults>(*inventory, ctx);
    }

    if (auto persistencySystem = aParser.LookupNodeData<modsave::PersistencySystemNode>())
    {
        if (auto garageComponent = persistencySystem->LookupChunk("vehicleGarageComponentPS"))
        {
            auto& ref = *garageComponent;
            auto& garage = Cast<GarageComponentPS>(ref);

            m_vehicleGarageResults = MakeHandle<VehicleGarageReader::VehicleGarageResults>(garage);
        }
    }

    if (auto wardrobeSystem = aParser.LookupNodeData<modsave::WardrobeSystemNode>())
    {
        m_wardrobeResults = MakeHandle<WardrobeReader::WardrobeResults>(*wardrobeSystem);
    }
}

void NGPlusProgressionData::PostProcess() noexcept
{
    // Clean up excessive attributes/perks
    constexpr auto c_minAttributeValue = 3;
    constexpr auto c_attributeCount = 5;

    constexpr auto c_maxAttributePointCount = (20 - c_minAttributeValue) * c_attributeCount;

    const auto allocatedAttributes = static_cast<int>(m_statsSystemResults->m_body + m_statsSystemResults->m_cool +
                                       m_statsSystemResults->m_intelligence + m_statsSystemResults->m_reflexes +
                                       m_statsSystemResults->m_technicalAbility) - c_minAttributeValue * c_attributeCount;

    const int totalAttributePoints = m_playerDevelopmentSystemResults->m_attributePoints + allocatedAttributes;

    if (totalAttributePoints > c_maxAttributePointCount && m_playerDevelopmentSystemResults->m_attributePoints > 0)
    {
        m_playerDevelopmentSystemResults->m_perkPoints += m_playerDevelopmentSystemResults->m_attributePoints;
        m_playerDevelopmentSystemResults->m_attributePoints = 0;
    }

    constexpr auto c_maxRelicPointCount = 15;

    if (m_playerDevelopmentSystemResults->m_relicPoints > c_maxRelicPointCount)
    {
        m_playerDevelopmentSystemResults->m_perkPoints +=
            (m_playerDevelopmentSystemResults->m_relicPoints - c_maxRelicPointCount);
        m_playerDevelopmentSystemResults->m_relicPoints = c_maxRelicPointCount;
    }

    constexpr auto c_maxPerkPointCount = 221;

    if (m_playerDevelopmentSystemResults->m_perkPoints > c_maxPerkPointCount)
    {
        m_playerInventory->m_money += (100 * m_playerDevelopmentSystemResults->m_perkPoints - c_maxPerkPointCount);
        m_playerDevelopmentSystemResults->m_perkPoints = c_maxPerkPointCount;
    }
}