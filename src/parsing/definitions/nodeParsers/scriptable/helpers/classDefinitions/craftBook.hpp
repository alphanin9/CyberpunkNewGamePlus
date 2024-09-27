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

    int GetAmount() const
    {
        return s_type->GetProperty("amount")->GetValue<int>(m_instance);
    }

    Red::DynArray<Red::ItemID> GetHideOnAdded() const
    {
        Red::DynArray<Red::ItemID> ret{};

        const auto prop = s_type->GetProperty("hideOnItemsAdded");
        
        auto type = static_cast<Red::CRTTIArrayType*>(prop->type);
        auto instancePtr = prop->GetValuePtr<void>(m_instance);

        const auto arrayLength = type->GetLength(instancePtr);

        ret.Reserve(arrayLength);

        for (auto i = 0u; i < arrayLength; i++)
        {
            auto elementPtr = reinterpret_cast<Red::ItemID*>(type->GetElement(instancePtr, i));

            ret.PushBack(*elementPtr);
        }

        return ret;
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

    inline operator bool() const
    {
        return m_instance != nullptr;
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