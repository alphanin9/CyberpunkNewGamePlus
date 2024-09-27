#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <context.hpp>

#include <scriptable/helpers/classDefinitions/playerDevelopmentData.hpp>

#include "playerDevelopmentSystemReader.hpp"

using namespace Red;

using scriptable::native::PlayerDevelopment::PlayerDevelopmentData;

// Enums
using scriptable::native::PlayerDevelopment::AttributeDataType;
using scriptable::native::PlayerDevelopment::EspionageMilestonePerks;

using scriptable::native::PlayerDevelopment::SAttribute;
using scriptable::native::PlayerDevelopment::SAttributeData;
using scriptable::native::PlayerDevelopment::SDevelopmentPoints;
using scriptable::native::PlayerDevelopment::SNewPerk;
using scriptable::native::PlayerDevelopment::SProficiency;

namespace PlayerDevelopmentSystemReader
{
PlayerDevelopmentData GetPlayerData(Handle<ISerializable>* aPlayerDevelopmentSystem) noexcept
{
    auto& instance = *aPlayerDevelopmentSystem;
    auto playerData = instance->GetType()->GetProperty("playerData");

    if (!playerData)
    {
        return nullptr;
    }

    auto& playerDataPtr = *playerData->GetValuePtr<DynArray<Handle<IScriptable>>>(instance);
    return playerDataPtr[0].GetPtr();
}

PlayerDevelopmentSystemResults GetData(Handle<ISerializable>* aPlayerDevelopmentSystem) noexcept
{
    PlayerDevelopmentSystemResults result{};

    auto data = GetPlayerData(aPlayerDevelopmentSystem);

    if (!data)
    {
        return result;
    }

    auto developmentPointIterator = [&result](SDevelopmentPoints aDevPoints)
    {
        auto spentCount = aDevPoints.GetSpent();
        auto unspentCount = aDevPoints.GetUnspent();

        using game::data::DevelopmentPointType;

        switch (aDevPoints.GetType())
        {
        case DevelopmentPointType::Espionage: // Relic
            result.m_playerRelicPoints += unspentCount;
            break;
        case DevelopmentPointType::Attribute:
            result.m_playerAttributePoints += unspentCount;
            break;
        case DevelopmentPointType::Primary: // Perks
            result.m_playerPerkPoints += unspentCount;
            break;
        default:
            break;
        }
    };

    const auto attributeDataIterator = [&result](SAttributeData aAttributeData)
    {
        auto isEspionage = aAttributeData.GetType() == AttributeDataType::EspionageAttributeData;

        const auto newPerkIterator = [&result, isEspionage](SNewPerk aNewPerk)
        {
            auto currLevel = aNewPerk.GetCurrLevel();

            if (currLevel == 0)
            {
                return;
            }

            if (isEspionage)
            {
                // Espionage doesn't have levels (it actually does, is there a point to this?)
                result.m_playerRelicPoints += aNewPerk.IsEspionageMilestonePerk() ? 3 : 1;
            }
            else
            {
                result.m_playerPerkPoints += currLevel;
            }
        };

        aAttributeData.IterateOverUnlockedPerks(newPerkIterator);
    };

    const auto proficiencyIterator = [&result](SProficiency aProficiency)
    {
        const auto currLevel = aProficiency.GetCurrentLevel();
        using game::data::ProficiencyType;
        switch (aProficiency.GetType())
        {
        case ProficiencyType::Level:
            result.m_playerLevel = std::clamp(currLevel, 1, 50);
            break;
        case ProficiencyType::StreetCred:
            result.m_playerStreetCred = currLevel;
            break;
        case ProficiencyType::StrengthSkill:
            result.m_playerBodySkillLevel = currLevel;
            break;
        case ProficiencyType::ReflexesSkill:
            result.m_playerReflexSkillLevel = currLevel;
            break;
        case ProficiencyType::TechnicalAbilitySkill:
            result.m_playerTechSkillLevel = currLevel;
            break;
        case ProficiencyType::IntelligenceSkill:
            result.m_playerIntelligenceSkillLevel = currLevel;
            break;
        case ProficiencyType::CoolSkill:
            result.m_playerCoolSkillLevel = currLevel;
            break;
        }
    };

    const auto attributeIterator = [&result](SAttribute aAttribute)
    {
        const auto currLevel = aAttribute.GetValue();

        using game::data::StatType;

        switch (aAttribute.GetAttributeName())
        {
        case StatType::Strength:
            result.m_playerBodyAttribute = currLevel;
            break;
        case StatType::Reflexes:
            result.m_playerReflexAttribute = currLevel;
            break;
        case StatType::TechnicalAbility:
            result.m_playerTechAttribute = currLevel;
            break;
        case StatType::Intelligence:
            result.m_playerIntelligenceAttribute = currLevel;
            break;
        case StatType::Cool:
            result.m_playerCoolAttribute = currLevel;
            break;
        }
    };

    data.IterateOverAttributes(attributeIterator);
    data.IterateOverAttributesData(attributeDataIterator);
    data.IterateOverDevPoints(developmentPointIterator);
    data.IterateOverProficiencies(proficiencyIterator);

    constexpr auto c_minAttributeValue = 3;
    constexpr auto c_attributeCount = 5;

    constexpr auto c_maxAttributePointCount = (20 - c_minAttributeValue) * c_attributeCount;

    const auto allocatedAttributePoints =
        (result.m_playerBodyAttribute + result.m_playerReflexAttribute + result.m_playerTechAttribute +
         result.m_playerIntelligenceAttribute + result.m_playerCoolAttribute) -
        c_minAttributeValue * c_attributeCount;

    const auto totalAttributePoints = allocatedAttributePoints + result.m_playerAttributePoints;

    if (totalAttributePoints > c_maxAttributePointCount && result.m_playerAttributePoints > 0)
    {
        result.m_playerPerkPoints += result.m_playerAttributePoints;
        result.m_playerAttributePoints = 0;
    }

    constexpr auto c_maxRelicPoints = 15;

    if (result.m_playerRelicPoints > c_maxRelicPoints)
    {
        result.m_playerPerkPoints += (result.m_playerRelicPoints - c_maxRelicPoints);
        result.m_playerRelicPoints = c_maxRelicPoints;
    }

    // https://old.reddit.com/r/LowSodiumCyberpunk/comments/1690oq4/mathed_out_the_new_perk_trees_coming_in_20_over/
    constexpr auto c_maxPerkPointCount = 41 + 46 + 44 + 42 + 46 + 2;
    // 219 is not enough, max seems to be 221...

    if (result.m_playerPerkPoints > c_maxPerkPointCount)
    {
        result.m_playerPerkPoints = c_maxPerkPointCount;
    }

    return result;
}
}