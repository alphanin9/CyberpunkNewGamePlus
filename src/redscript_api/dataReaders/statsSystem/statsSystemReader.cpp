#include "statsSystemReader.hpp"
#include "../util/offsetPtr.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>

using namespace Red;
using game::data::StatType;

StatsSystemReader::StatsSystemResults::StatsSystemResults(ResultContext& aContext) noexcept
{
    constexpr auto c_playerEntityId = 1ull;

    for (auto& i : aContext.m_statsSystem->GetStatModifiers(c_playerEntityId))
    {
        auto& asConstant = Cast<game::ConstantStatModifierData_Deprecated>(i);
        constexpr auto c_bigCWCapValue = 250.f;
        if (asConstant && asConstant->modifierType == game::StatModifierType::Additive &&
            asConstant->statType == StatType::Humanity && asConstant->value < c_bigCWCapValue)
        {
            m_cyberwareCapacity.PushBack(asConstant->value);
        }
    }

    for (auto& i : aContext.m_statsSystem->GetForcedModifiers(c_playerEntityId))
    {
        auto& asConstant = Cast<game::ConstantStatModifierData_Deprecated>(i);

        if (!asConstant)
        {
            continue;
        }

        if (asConstant->modifierType != game::StatModifierType::Additive)
        {
            continue;
        }

        switch (asConstant->statType)
        {
        case StatType::CoolSkill:
            m_coolSkill = asConstant->value;
            break;
        case StatType::IntelligenceSkill:
            m_intelligenceSkill = asConstant->value;
            break;
        case StatType::ReflexesSkill:
            m_reflexesSkill = asConstant->value;
            break;
        case StatType::StreetCred:
            m_streetCred = asConstant->value;
            break;
        case StatType::StrengthSkill:
            m_bodySkill = asConstant->value;
            break;
        case StatType::TechnicalAbilitySkill:
            m_technicalAbilitySkill = asConstant->value;
            break;
        case StatType::Cool:
            m_cool = asConstant->value;
            break;
        case StatType::Intelligence:
            m_intelligence = asConstant->value;
            break;
        case StatType::Reflexes:
            m_reflexes = asConstant->value;
            break;
        case StatType::Strength:
            m_body = asConstant->value;
            break;
        case StatType::TechnicalAbility:
            m_technicalAbility = asConstant->value;
            break;
        case StatType::Level:
            const auto gameEngine = CGameEngine::Get();

            auto isEP1 = false;

            // IsEP1() has this weird check
            if (gameEngine)
            {
                isEP1 = util::OffsetPtr<724u, bool>::Ref(gameEngine);
            }

            constexpr auto c_ep1MaxLevel = 50.f;
            constexpr auto c_nonEp1MaxLevel = 40.f;

            // Keep the non-EP1 clamp...
            // Oops, wrong func!
            m_level = std::min(asConstant->value, isEP1 ? c_ep1MaxLevel : c_nonEp1MaxLevel);
            break;
        }
    }
}