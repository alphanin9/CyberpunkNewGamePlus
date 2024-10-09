#include "statsSystemReader.hpp"
#include "../util/offsetPtr.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>

using namespace Red;
using game::data::StatType;

StatsSystemReader::StatsSystemResults::StatsSystemResults(ResultContext& aContext) noexcept
{
    constexpr auto c_playerEntityId = 1ull;

    const auto gameEngine = CGameEngine::Get();

    auto isEP1 = false;

    // IsEP1() has this weird check
    if (gameEngine)
    {
        isEP1 = util::OffsetPtr<724u, bool>::Ref(gameEngine);
    }
        
    auto savedModifiers = aContext.m_statsSystem->GetStatModifiers(c_playerEntityId);
    auto forcedModifiers = aContext.m_statsSystem->GetForcedModifiers(c_playerEntityId);

    for (auto& i : savedModifiers)
    {
        auto& asConstant = Cast<game::ConstantStatModifierData_Deprecated>(i);
        constexpr auto c_bigCWCapValue = 250.f;
        if (asConstant && asConstant->modifierType == game::StatModifierType::Additive &&
            asConstant->statType == StatType::Humanity && asConstant->value < c_bigCWCapValue)
        {
            m_cyberwareCapacity.PushBack(asConstant->value);
        }
    }

    for (auto& i : forcedModifiers)
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
            // Keep the non-EP1 clamp...
            m_level = std::max(asConstant->value, isEP1 ? 50.f : 40.f);
            break;
        }
    }
}