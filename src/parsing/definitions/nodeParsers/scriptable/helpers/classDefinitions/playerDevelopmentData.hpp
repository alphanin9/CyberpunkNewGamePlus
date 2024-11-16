#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/data/StatType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/DevelopmentPointType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ProficiencyType.hpp>

namespace scriptable::native::PlayerDevelopment
{
using RawPointerToValue = std::remove_pointer_t<Red::ScriptInstance>;

using ScriptableEnum = std::uint32_t;

enum class AttributeDataType : ScriptableEnum
{
    BodyAttributeData = 0,
    CoolAttributeData = 1,
    EspionageAttributeData = 2,
    IntelligenceAttributeData = 3,
    ReflexesAttributeData = 4,
    TechnicalAbilityAttributeData = 5,
    Count = 6,
    Invalid = 7
};

enum class EspionageMilestonePerks : ScriptableEnum
{
    Espionage_Central_Milestone1 = 71,
    Espionage_Left_Milestone_Perk = 76,
    Espionage_Right_Milestone_1 = 78
};

struct SNewPerk
{
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;

    inline SNewPerk(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<SNewPerk>();
    }

    inline int GetCurrLevel()
    {
        return s_type->GetProperty("currLevel")->GetValue<int>(m_instance);
    }

    inline std::uint32_t GetType()
    {
        return s_type->GetProperty("type")->GetValue<std::uint32_t>(m_instance);
    }

    inline bool IsEspionageMilestonePerk()
    {
        const auto type = GetType();

        return type == static_cast<std::uint32_t>(EspionageMilestonePerks::Espionage_Central_Milestone1) ||
               type == static_cast<std::uint32_t>(EspionageMilestonePerks::Espionage_Left_Milestone_Perk) ||
               type == static_cast<std::uint32_t>(EspionageMilestonePerks::Espionage_Right_Milestone_1);
    }
};

struct SAttributeData
{
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;

    inline SAttributeData(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<SAttributeData>();
    }

    inline AttributeDataType GetType()
    {
        return s_type->GetProperty("type")->GetValue<AttributeDataType>(m_instance);
    }

    // Templated to allow easy lambdas without bringing in std::function
    template<typename Fn>
    inline void IterateOverUnlockedPerks(Fn fn)
    {
        const auto prop = s_type->GetProperty("unlockedPerks");
        const auto arrayType = static_cast<Red::CRTTIArrayType*>(prop->type);
        const auto arrayPtr = prop->GetValuePtr<RawPointerToValue>(m_instance);

        for (auto i = 0u; i < arrayType->GetLength(arrayPtr); i++)
        {
            fn(SNewPerk{arrayType->GetElement(arrayPtr, i)});
        }
    }
};

struct SAttribute
{
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;
    
    inline SAttribute(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<SAttribute>();
    }

    inline Red::gamedataStatType GetAttributeName()
    {
        return s_type->GetProperty("attributeName")->GetValue<Red::gamedataStatType>(m_instance);
    }

    inline Red::TweakDBID GetId()
    {
        return s_type->GetProperty("id")->GetValue<Red::TweakDBID>(m_instance);
    }

    inline int GetValue()
    {
        return s_type->GetProperty("value")->GetValue<int>(m_instance);
    }
};

struct SDevelopmentPoints
{
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;

    inline SDevelopmentPoints(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<SDevelopmentPoints>();
    }

    inline int GetSpent()
    {
        return s_type->GetProperty("spent")->GetValue<int>(m_instance);
    }

    inline int GetUnspent()
    {
        return s_type->GetProperty("unspent")->GetValue<int>(m_instance);
    }

    inline Red::gamedataDevelopmentPointType GetType()
    {
        return s_type->GetProperty("type")->GetValue<Red::gamedataDevelopmentPointType>(m_instance);
    }
};

struct SProficiency
{
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;

    inline SProficiency(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<SProficiency>();
    }

    inline int GetCurrentLevel()
    {
        return s_type->GetProperty("currentLevel")->GetValue<int>(m_instance);
    }

    inline Red::gamedataProficiencyType GetType()
    {
        return s_type->GetProperty("type")->GetValue<Red::gamedataProficiencyType>(m_instance);
    }
};

class PlayerDevelopmentData {
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;
public:
    inline PlayerDevelopmentData(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<PlayerDevelopmentData>();
    }

    inline operator bool() const
    {
        return m_instance != nullptr;
    }

    inline Red::DynArray<SAttribute> GetAttributes()
    {
        const auto attributeProp = s_type->GetProperty("attributes");
        const auto arrayType = static_cast<Red::CRTTIArrayType*>(attributeProp->type);
        const auto attributePtr = s_type->GetProperty("attributes")->GetValuePtr<RawPointerToValue>(m_instance);

        Red::DynArray<SAttribute> ret{};

        for (auto i = 0u; i < arrayType->GetLength(attributePtr); i++)
        {
            auto elem = arrayType->GetElement(attributePtr, i);

            ret.PushBack(SAttribute{elem});
        }
        
        return ret;
    }

    template<Red::CName propertyName, typename Fn, typename NativeType>
    inline void IterateOver(Fn fn)
    {
        const auto prop = s_type->GetProperty(propertyName);
        const auto arrayPtr = prop->GetValuePtr<RawPointerToValue>(m_instance);
        const auto arrayType = static_cast<Red::CRTTIBaseArrayType*>(prop->type);

        for (auto i = 0u; i < arrayType->GetLength(arrayPtr); i++)
        {
            fn(NativeType{arrayType->GetElement(arrayPtr, i)});
        }
    }

    template<typename Fn>
    inline void IterateOverAttributes(Fn fn)
    {
        return IterateOver<"attributes", Fn, SAttribute>(fn);
    }

    template<typename Fn>
    inline void IterateOverProficiencies(Fn fn)
    {
        return IterateOver<"proficiencies", Fn, SProficiency>(fn);
    }

    template<typename Fn>
    inline void IterateOverAttributesData(Fn fn)
    {
        return IterateOver<"attributesData", Fn, SAttributeData>(fn);
    }

    template<typename Fn>
    inline void IterateOverDevPoints(Fn fn)
    {
        return IterateOver<"devPoints", Fn, SDevelopmentPoints>(fn);
    }
};
}
