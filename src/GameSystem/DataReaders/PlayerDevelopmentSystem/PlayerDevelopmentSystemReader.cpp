#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <context/context.hpp>
#include <parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/playerDevelopmentData.hpp>

#include "playerDevelopmentSystemReader.hpp"

#include <Shared/RTTI/PropertyAccessor.hpp>

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

PlayerDevelopmentSystemReader::PlayerDevelopmentSystemResults::PlayerDevelopmentSystemResults(
    Handle<ISerializable>* aPlayerDevelopmentSystem) noexcept
{
    auto& playerOwnerData =
        shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "playerData">(*aPlayerDevelopmentSystem);

    PlayerDevelopmentData playerDevelopmentData = playerOwnerData[0].instance;

    auto developmentPointIterator = [this](SDevelopmentPoints aDevPoints)
    {
        auto unspentCount = aDevPoints.GetUnspent();

        using game::data::DevelopmentPointType;

        switch (aDevPoints.GetType())
        {
        case DevelopmentPointType::Espionage: // Relic
            m_relicPoints += unspentCount;
            break;
        case DevelopmentPointType::Attribute:
            m_attributePoints += unspentCount;
            break;
        case DevelopmentPointType::Primary: // Perks
            m_perkPoints += unspentCount;
            break;
        default:
            break;
        }
    };

    const auto attributeDataIterator = [this](SAttributeData aAttributeData)
    {
        auto isEspionage = aAttributeData.GetType() == AttributeDataType::EspionageAttributeData;

        const auto newPerkIterator = [this, isEspionage](SNewPerk aNewPerk)
        {
            auto currLevel = aNewPerk.GetCurrLevel();

            if (currLevel == 0)
            {
                return;
            }

            if (isEspionage)
            {
                // Espionage doesn't have levels (it actually does, is there a point to this?)
                m_relicPoints += aNewPerk.IsEspionageMilestonePerk() ? 3 : 1;
            }
            else
            {
                m_perkPoints += currLevel;
            }
        };

        aAttributeData.IterateOverUnlockedPerks(newPerkIterator);
    };

    playerDevelopmentData.IterateOverAttributesData(attributeDataIterator);
    playerDevelopmentData.IterateOverDevPoints(developmentPointIterator);
}