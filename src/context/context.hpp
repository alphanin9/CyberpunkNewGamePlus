#pragma once
#include <format>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace PluginContext {
	inline bool m_isInStartNewGame{};
	inline bool m_isNewGamePlusActive{};

	inline Red::PluginHandle m_redPlugin;
	inline const Red::Sdk* m_redSdk;

	inline std::uint64_t m_ngPlusGameDefinitionHash = Red::FNV1a64("base\\quest\\newgameplus.gamedef");
    inline Red::CRTTISystem* m_rtti;


    template<typename... Args>
    inline void Spew(std::string_view aStr, Args&&... aArgs)
    {
        auto str = std::vformat(aStr, std::make_format_args(aArgs...));
        
        Red::Log::Debug("{}", str);
        m_redSdk->logger->Info(m_redPlugin, str.c_str());
	}


    template<typename... Args>
    inline void Error(std::string_view aStr, Args&&... aArgs)
    {
        auto str = std::vformat(aStr, std::make_format_args(aArgs...));

        Red::Log::Debug("Error: {}", str);
        m_redSdk->logger->Error(m_redPlugin, str.c_str());
	}
    }