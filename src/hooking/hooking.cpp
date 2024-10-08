#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <context.hpp>

#include <MinHook.h>

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
			if (!PluginContext::m_isNewGamePlusActive || !PluginContext::m_isInStartNewGame) {
				return m_originalFn(aDepotPath, aGamedefType);
			}

			// Fix for non-EP1 NG+ start
			if (aGamedefType >= GamedefType::EP1_Standalone) {
				return m_originalFn(aDepotPath, aGamedefType);
			}

			*aDepotPath = PluginContext::m_ngPlusGameDefinitionHash;

			PluginContext::m_isNewGamePlusActive = false;
			return aDepotPath;
		}
	}

	namespace StartNewGame {
		constexpr auto m_fnHash = 3897433288u;

		using StartNewGame = void(__fastcall*)(uintptr_t aThis, uintptr_t aState);
		StartNewGame m_originalFn = nullptr;

		void __fastcall m_detourFn(uintptr_t aThis, uintptr_t aState) {
			PluginContext::m_isInStartNewGame = true;
			m_originalFn(aThis, aState);
			PluginContext::m_isInStartNewGame = false;
		}
	}

	bool InitializeHooking() {
		const auto addrSelectGameDefinition = Red::UniversalRelocBase::Resolve(SelectGameDefinition::m_fnHash);
        const auto addrStartNewGame = Red::UniversalRelocBase::Resolve(StartNewGame::m_fnHash);

		if (MH_Initialize() != MH_OK)
        {
            return false;
		}

		if (MH_CreateHook(reinterpret_cast<void*>(addrSelectGameDefinition), SelectGameDefinition::m_detourFn,
                          reinterpret_cast<void**>(&SelectGameDefinition::m_originalFn)) != MH_OK)
        {
            return false;
		}

		if (MH_CreateHook(reinterpret_cast<void*>(addrStartNewGame), StartNewGame::m_detourFn,
                          reinterpret_cast<void**>(&StartNewGame::m_originalFn)) != MH_OK)
        {
            return false;
		}

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        {
            return false;
		}

		return true;
	}

	bool DetachHooking() {
		auto status = MH_DisableHook(MH_ALL_HOOKS) == MH_OK;
        
		return status && MH_Uninitialize() == MH_OK;
	}
}