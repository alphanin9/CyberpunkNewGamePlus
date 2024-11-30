#pragma once
#include <format>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <Shared/Util/NamePoolRegistrar.hpp>

namespace PluginContext
{
inline Red::PluginHandle m_redPlugin;
inline const Red::Sdk* m_redSdk;

inline Red::CRTTISystem* m_rtti;

// Probably fine as not atomic...
inline bool m_rttiReady{};

// Enable enhanced logging
inline bool m_isDebugModeEnabled{};

template<typename... Args>
inline void Spew(std::string_view aStr, Args&&... aArgs)
{
    auto str = std::vformat(aStr, std::make_format_args(aArgs...));
    if (m_rttiReady)
    {
        Red::Log::Channel(shared::util::NamePoolRegistrar<"New Game+">::Get(), str);
    }
    m_redSdk->logger->Info(m_redPlugin, str.c_str());
}

template<typename... Args>
inline void Error(std::string_view aStr, Args&&... aArgs)
{
    auto str = std::vformat(aStr, std::make_format_args(aArgs...));
    if (m_rttiReady)
    {
        Red::Log::Channel(shared::util::NamePoolRegistrar<"New Game+ | Error">::Get(), str);
    }
    m_redSdk->logger->Error(m_redPlugin, str.c_str());
}

template<typename ...Args>
inline void DebugLog(std::string_view aStr, Args&&... aArgs)
{
    // Only run anything in debug mode
    if (!m_isDebugModeEnabled)
    {
        return;
    }

    auto str = std::vformat(aStr, std::make_format_args(aArgs...));
    if (m_rttiReady)
    {
        Red::Log::Channel(shared::util::NamePoolRegistrar<"New Game+ | Debug">::Get(), str);
    }
    m_redSdk->logger->Error(m_redPlugin, str.c_str());
}
} // namespace PluginContext