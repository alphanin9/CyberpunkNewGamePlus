#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace scriptable::native::CraftBook
{
using RawPointerToValue = std::remove_pointer_t<Red::ScriptInstance>;

struct ItemRecipe
{
    inline static Red::CClass* s_type;
    Red::ScriptInstance m_instance;

    ItemRecipe(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        if (!s_type)
        {
            s_type = Red::GetClass<ItemRecipe>();
        }
    }

    Red::TweakDBID GetTargetItem() const
    {
        return s_type->GetProperty("targetItem")->GetValue<Red::TweakDBID>(m_instance);
    }
};

struct CraftBook
{
    inline static Red::CClass* s_type;
    Red::ScriptInstance m_instance;

    CraftBook(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        if (!s_type)
        {
            s_type = Red::GetClass<CraftBook>();
        }
    }

    template<Red::CName aPropertyName, typename Fn, typename NativeType>
    inline void IterateOver(Fn fn)
    {
        const auto prop = s_type->GetProperty(aPropertyName);
        const auto arrayPtr = prop->GetValuePtr<RawPointerToValue>(m_instance);
        const auto arrayType = static_cast<Red::CRTTIBaseArrayType*>(prop->type);

        for (auto i = 0u; i < arrayType->GetLength(arrayPtr); i++)
        {
            fn(NativeType{arrayType->GetElement(arrayPtr, i)});
        }
    }

    template<typename Fn>
    inline void IterateOverRecipes(Fn fn)
    {
        return IterateOver<"knownRecipes", Fn, ItemRecipe>(fn);
    }
};
} // namespace scriptable::native::EquipmentSystem