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

    inline void Spew(std::string_view aStr)
    {
        Red::Log::Debug("{}", aStr);
        m_redSdk->logger->Trace(m_redPlugin, aStr.data());
	}

    inline void Error(std::string_view aStr)
    {
        Red::Log::Debug("Error: {}", aStr);
        m_redSdk->logger->Error(m_redPlugin, aStr.data());
	}
    }