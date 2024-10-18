#pragma once
#include <format>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>
#include <filesystem>

namespace PluginContext {
	inline bool m_isInStartNewGame{};
	inline bool m_isNewGamePlusActive{};

	inline Red::PluginHandle m_redPlugin;
	inline const Red::Sdk* m_redSdk;

	inline std::uint64_t m_ngPlusGameDefinitionHash = Red::FNV1a64("base\\quest\\newgameplus.gamedef");
    inline Red::CRTTISystem* m_rtti;

    // Probably fine as not atomic...
    inline bool m_rttiReady{};

    template<typename... Args>
    inline void Spew(std::string_view aStr, Args&&... aArgs)
    {
        auto str = std::vformat(aStr, std::make_format_args(aArgs...));
        if (m_rttiReady)
        {
            Red::Log::Channel("New Game+", str);
        }
        m_redSdk->logger->Info(m_redPlugin, str.c_str());
	}

    template<typename... Args>
    inline void Error(std::string_view aStr, Args&&... aArgs)
    {
        auto str = std::vformat(aStr, std::make_format_args(aArgs...));
        if (m_rttiReady)
        {
            Red::Log::Channel("New Game+ | Error", str);
        }
        m_redSdk->logger->Error(m_redPlugin, str.c_str());
	}

    inline void RegisterPluginChannelNames()
    {
        // Won't pick up in CET game log without this
        Red::CNamePool::Add("New Game+");
        Red::CNamePool::Add("New Game+ | Error");
    }
    }