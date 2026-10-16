// macOS: the GameInstance and the late registration of New Game Plus's game system.
//
// The engine's framework is not at the SDK's +0x308 on macOS, so RedLib reaches the GameInstance through Codeware's
// macOS port (codeware_mac_game_instance), as the Let There Be Flight and CET ports do. The GameInstance is created after
// the scripts load; RedLib re-queues the system registration until then. Codeware hooks the GameInstance constructor
// and runs its own queue there; New Game Plus chains onto that hook (codeware_mac_hook_chain_attach, one hook per
// target) and runs its queue right after the constructor returns.

#include "MacShims.hpp"

#include <RED4ext/GameEngine.hpp>
#include <RedLib.hpp>

#include <dlfcn.h>

#include <atomic>
#include <cstdint>

namespace
{
// The GameInstance constructor Codeware hooks (cp2077-codeware macos/MacSupport.cpp, kGameInstanceCtorRva).
constexpr uintptr_t kGameInstanceCtorRva = 0x1F2D640;

using CtorFn = void* (*)(void*);
CtorFn s_originalCtor = nullptr;
std::atomic<bool> s_chained{false};

void* OnGameInstanceCtor(void* aThis)
{
    void* result = s_originalCtor ? s_originalCtor(aThis) : nullptr;
    if (auto* engine = Red::CGameEngine::Get(); engine && engine->framework && engine->framework->gameInstance)
    {
        Red::TypeInfoRegistrar::RunPending();
        NgpMac::Log("[ngplus] GameInstance ready: late registration ran");
    }
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
