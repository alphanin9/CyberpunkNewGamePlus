#pragma once
#include <RED4ext/RED4ext.hpp>

namespace pluginContext {
	inline bool m_isInStartNewGame{};
	inline bool m_isSecretOverrideActivated{};

	inline RED4ext::PluginHandle m_redPlugin;
	inline const RED4ext::Sdk* m_redSdk;

	inline std::uint64_t m_ngPlusGameDefinitionHash = RED4ext::FNV1a64("base\\quest\\newgameplus.gamedef");
}