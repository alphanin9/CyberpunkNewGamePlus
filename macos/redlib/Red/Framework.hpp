#pragma once

#include "TypeInfo/Resolving.hpp"

namespace Red
{
#if defined(__APPLE__)
// macOS: from Codeware's codeware_mac_game_instance (see TypeInfo/Definition.hpp). The engine's framework is not at the
// SDK's +0x308 on macOS.
::RED4ext::GameInstance* MacGameInstance();
#endif

template<typename T, typename U = T>
requires std::is_base_of_v<IGameSystem, T> && std::is_base_of_v<T, U>
U* GetGameSystem()
{
    static const auto s_type = GetType<T>();
#if defined(__APPLE__)
    auto* gameInstance = MacGameInstance();
    if (!gameInstance)
    {
        return nullptr;
    }
#else
    auto& gameInstance = CGameEngine::Get()->framework->gameInstance;
#endif
    return reinterpret_cast<U*>(gameInstance->GetSystem(s_type));
}
}
