#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../resultContext.hpp"

namespace StatsSystemReader
{
class StatsSystemResults : public Red::IScriptable
{
public:
    Red::DynArray<float> m_cyberwareCapacity{};

    float m_level{};
    float m_streetCred{};
    
    float m_reflexes{};
    float m_body{};
    float m_technicalAbility{};
    float m_intelligence{};
    float m_cool{};

    float m_reflexesSkill{};
    float m_bodySkill{};
    float m_technicalAbilitySkill{};
    float m_intelligenceSkill{};
    float m_coolSkill{};

    StatsSystemResults() = default;
    StatsSystemResults(ResultContext& aContext) noexcept;

    RTTI_IMPL_TYPEINFO(StatsSystemResults);
    RTTI_IMPL_ALLOCATOR();
};
}

RTTI_DEFINE_CLASS(StatsSystemReader::StatsSystemResults, {
    RTTI_GETTER(m_cyberwareCapacity);
    RTTI_GETTER(m_level);
    RTTI_GETTER(m_streetCred);
    RTTI_GETTER(m_reflexes);
    RTTI_GETTER(m_body);
    RTTI_GETTER(m_technicalAbility);
    RTTI_GETTER(m_intelligence);
    RTTI_GETTER(m_cool);
    RTTI_GETTER(m_reflexesSkill);
    RTTI_GETTER(m_bodySkill);
    RTTI_GETTER(m_technicalAbilitySkill);
    RTTI_GETTER(m_intelligenceSkill);
    RTTI_GETTER(m_coolSkill);
});