#pragma once
namespace util
{
// Sets m_value to m_setTo on destructor if m_enabled == true
template<typename T>
struct ScopeGuard
{
    ~ScopeGuard()
    {
        if (m_enabled)
        {
            m_value = m_setTo;
        }
    }

    void SetEnabled(bool aEnabled)
    {
        m_enabled = aEnabled;
    }

    ScopeGuard(T& aValue, T aSetTo)
        : m_enabled(true)
        , m_value(aValue)
        , m_setTo(aSetTo)
    {
    }

    bool m_enabled;
    T& m_value;
    T m_setTo;
};
} // namespace util