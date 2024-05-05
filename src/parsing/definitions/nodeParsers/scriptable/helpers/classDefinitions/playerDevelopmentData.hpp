#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/data/StatType.hpp>

namespace scriptable::native::test
{
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

class PlayerDevelopmentData {
    Red::ScriptInstance m_instance;
    inline static Red::CClass* s_type;
public:
    inline PlayerDevelopmentData(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        s_type = Red::GetClass<PlayerDevelopmentData>();
    }

    inline Red::DynArray<SAttribute> GetAttributes()
    {
        const auto attributeProp = s_type->GetProperty("attributes");
        const auto arrayType = static_cast<Red::CRTTIArrayType*>(attributeProp->type);
        const auto attributePtr =
            s_type->GetProperty("attributes")->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(m_instance);

        Red::DynArray<SAttribute> ret{};

        const auto arrayLength = arrayType->GetLength(attributePtr);

        for (auto i = 0; i < arrayLength; i++)
        {
            auto elem = arrayType->GetElement(attributePtr, i);

            ret.PushBack(SAttribute{elem});
        }
        
        return ret;
    }
};
}
