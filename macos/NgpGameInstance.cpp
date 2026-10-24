// macOS: the late registration of New Game Plus's game system.
//
// The game creates the GameInstance after the scripts load, inside the framework's constructor; the engine stores the
// framework (CGameEngine +0x338) only afterwards. RedLib's RegisterSystem reads the GameInstance through that framework,
// so it re-queues itself and nothing runs the queue again. Codeware hooks the GameInstance constructor (its own systems
// register there); New Game Plus chains onto that hook (codeware_mac_hook_chain_attach, one hook per target) and puts its
// system into the new GameInstance itself, the steps RedLib's RegisterSystem takes. Once the framework is stored, RedLib's
// re-queued callback finds the system in the map and returns.

#include "MacShims.hpp"

#include <RED4ext/GameEngine.hpp>
#include <RedLib.hpp>

#include <GameSystem/NewGamePlusSystem.hpp>

#include <dlfcn.h>

#include <atomic>
#include <cstdint>

namespace
{
// The GameInstance constructor Codeware hooks (cp2077-codeware macos/MacSupport.cpp, kGameInstanceCtorRva).
constexpr uintptr_t kGameInstanceCtorRva = 0x1691AEC;

using CtorFn = void* (*)(void*);
CtorFn s_originalCtor = nullptr;
std::atomic<bool> s_chained{false};

void* OnGameInstanceCtor(void* aThis)
{
    void* result = s_originalCtor ? s_originalCtor(aThis) : nullptr;
    auto* gameInstance = static_cast<Red::GameInstance*>(aThis);
    // By name: this file does not see the RTTI_DEFINE_CLASS name mapping of NewGamePlusSystem.cpp, so GetClass<T> here
    // would fall back to the SDK base's name (gameIGameSystem).
    auto* systemType = Red::CRTTISystem::Get()->GetClass("NewGamePlusSystem");
    if (!gameInstance || !systemType)
    {
        NgpMac::Log("[ngplus] GameInstance ready: system NOT registered (instance %p, type %p)", aThis,
                    static_cast<void*>(systemType));
        return result;
    }
    if (gameInstance->systemMap.Get(systemType))
    {
        return result;
    }
    auto system = Red::MakeHandle<mod::NewGamePlusSystem>();
    Red::JobHandle job{};
    system->gameInstance = gameInstance;
    system->OnInitialize(job);
    gameInstance->systemMap.Insert(systemType, system);
    gameInstance->systemImplementations.Insert(systemType, systemType);
    gameInstance->systemInstances.EmplaceBack(static_cast<Red::Handle<Red::IScriptable>&>(system));
    NgpMac::Log("[ngplus] GameInstance ready: NewGamePlusSystem registered");
    return result;
}
} // namespace

namespace NgpMac
{
bool ChainGameInstanceCtor()
{
    if (s_chained.exchange(true))
    {
        return true;
    }
    using AttachFn = bool (*)(uintptr_t, void*, void**);
    const auto attach = reinterpret_cast<AttachFn>(dlsym(RTLD_DEFAULT, "codeware_mac_hook_chain_attach"));
    if (!attach)
    {
        Log("[ngplus] GameInstance chain SKIPPED: Codeware for macOS (codeware_mac_hook_chain_attach) not found");
        return false;
    }
    void* original = nullptr;
    const bool ok = attach(ImageBase() + kGameInstanceCtorRva, reinterpret_cast<void*>(&OnGameInstanceCtor), &original);
    if (!ok || !original)
    {
        Log("[ngplus] GameInstance chain SKIPPED: Codeware has no hook on rva=0x%lx",
            static_cast<unsigned long>(kGameInstanceCtorRva));
        return false;
    }
    s_originalCtor = reinterpret_cast<CtorFn>(original);
    Log("[ngplus] GameInstance chain INSTALLED after Codeware's hook (rva=0x%lx)",
        static_cast<unsigned long>(kGameInstanceCtorRva));
    return true;
}
} // namespace NgpMac
