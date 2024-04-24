#include <RED4ext/RED4ext.hpp>

#include "../context/context.hpp"

#include <print>

namespace hooking {
	namespace SelectGameDefinition {
		constexpr auto m_fnHash = 2680298706u;

		enum class GamedefType : char
		{
			Basegame = 0,
			EP1 = 1,
			EP1_Standalone = 2,
			GamedefTypesMax = 3
		};

		using SelectGameDefinition = uint64_t*(__fastcall*)(uint64_t* aDepotPath, GamedefType aGamedefType);
		SelectGameDefinition m_originalFn = nullptr;

		uint64_t* __fastcall m_detourFn(uint64_t* aDepotPath, GamedefType aGamedefType) {
			if (!pluginContext::m_isSecretOverrideActivated || !pluginContext::m_isInStartNewGame) {
				return m_originalFn(aDepotPath, aGamedefType);
			}

			if (aGamedefType != GamedefType::EP1) {
				return m_originalFn(aDepotPath, aGamedefType);
			}

			*aDepotPath = pluginContext::m_ngPlusGameDefinitionHash;

			pluginContext::m_isSecretOverrideActivated = false;
			return aDepotPath;
		}
	}

	namespace StartNewGame {
		constexpr auto m_fnHash = 3897433288u;

		using StartNewGame = void(__fastcall*)(uintptr_t aState);
		StartNewGame m_originalFn = nullptr;

		void __fastcall m_detourFn(uintptr_t aState) {
			pluginContext::m_isInStartNewGame = true;
			m_originalFn(aState);
			pluginContext::m_isInStartNewGame = false;
		}
	}

	bool InitializeHooking() {
		const auto addrSelectGameDefinition =
			RED4ext::UniversalRelocPtr<uintptr_t>(SelectGameDefinition::m_fnHash).GetAddr();
		const auto addrStartNewGame = RED4ext::UniversalRelocPtr<uintptr_t>(StartNewGame::m_fnHash).GetAddr();

		auto hookStatus = pluginContext::m_redSdk->hooking->Attach(pluginContext::m_redPlugin, addrSelectGameDefinition, SelectGameDefinition::m_detourFn, reinterpret_cast<void**>(&SelectGameDefinition::m_originalFn));

		hookStatus = hookStatus && pluginContext::m_redSdk->hooking->Attach(pluginContext::m_redPlugin, addrStartNewGame, StartNewGame::m_detourFn, reinterpret_cast<void**>(&StartNewGame::m_originalFn));

		return hookStatus;
	}

	bool DetachHooking() {
		const auto addrSelectGameDefinition =
			RED4ext::UniversalRelocPtr<uintptr_t>(SelectGameDefinition::m_fnHash).GetAddr();
		const auto addrStartNewGame = RED4ext::UniversalRelocPtr<uintptr_t>(StartNewGame::m_fnHash).GetAddr();

		return pluginContext::m_redSdk->hooking->Detach(pluginContext::m_redPlugin, addrSelectGameDefinition) && pluginContext::m_redSdk->hooking->Detach(pluginContext::m_redPlugin, addrStartNewGame);
	}
}