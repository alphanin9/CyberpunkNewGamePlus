#pragma once
#include <RedLib.hpp>

// Copypasted from psiberx
namespace red
{
struct EventCallback
{
    EventCallback(const Red::WeakHandle<Red::IScriptable>& aObject, Red::CName aFunction, bool aSticky)
        : m_object(aObject)
        , m_function(aFunction)
    {
    }

    EventCallback(Red::CName aType, Red::CName aFunction, bool aSticky)
        : m_staticType(aType)
        , m_function(aFunction)
    {
    }

    EventCallback()
    {
    }

    template<typename... Args>
    void FireCallback(Args&&... aArgs) const
    {
        if (!m_staticType.IsNone())
        {
            Red::CallStatic(m_staticType, m_function, std::forward<Args>(aArgs)...);
        }
        else if (!m_object.Expired())
        {
            Red::CallVirtual(m_object.Lock(), m_function, std::forward<Args>(aArgs)...);
        }
    }

    bool operator==(const EventCallback& aRhs) const noexcept
    {
        return m_object.instance == aRhs.m_object.instance && m_staticType == aRhs.m_staticType &&
               m_function == aRhs.m_function;
    }

    Red::WeakHandle<Red::IScriptable> m_object{};
    Red::CName m_staticType{};
    Red::CName m_function{};
};
}