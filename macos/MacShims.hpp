#pragma once

// macOS layer for New Game Plus. Windows code keeps the upstream paths; Apple calls here.

#include <cstdint>
#include <filesystem>

#include <RED4ext/Api/v1/PluginHandle.hpp>
#include <RED4ext/Api/v1/Sdk.hpp>

extern "C"
{
// An address hash with no Mac row: logs once and returns a stub that logs its first call and returns 0. The SDK calls it
// for such a hash (RED4EXT_MAC_UNKNOWN_HASH_HANDLER in MacSdkSupport.cpp).
uintptr_t ngplus_mac_trap_for_hash(uint32_t aHash);

// Game image base (the executable's Mach-O header). SharedPunk's RawBase calls this on Apple.
uintptr_t sharedpunk_mac_image_base();

// False for 0 and for the trap stub. True for a resolved game address.
bool sharedpunk_mac_address_is_real(uintptr_t aAddress);

// One line per hash. SharedPunk calls this when a missing hash would have been called or read.
void sharedpunk_mac_log_missing_hash(uint32_t aHash);

// One attach or detach through the loader hooking API. The SDK is the one SetPlugin stored.
bool sharedpunk_mac_attach(uintptr_t aAddr, void* aDetour, void** aOriginal);
bool sharedpunk_mac_detach(uintptr_t aAddr);
}

namespace NgpMac
{
uintptr_t ImageBase();

// Chains onto Codeware's GameInstance constructor hook so the queued game-system registration runs once the GameInstance
// exists (NgpGameInstance.cpp). True when the chain is in place.
bool ChainGameInstanceCtor();
bool IsTrap(uintptr_t aAddress);

std::filesystem::path PluginDir();
std::filesystem::path RootDir();

void Log(const char* aFormat, ...) __attribute__((format(printf, 1, 2)));

void SetPlugin(const RED4ext::v1::Sdk* aSdk, RED4ext::v1::PluginHandle aHandle);
const RED4ext::v1::Sdk* PluginSdk();
RED4ext::v1::PluginHandle PluginHandleValue();

// One attach attempt. Logs INSTALLED, or SKIPPED when the attach does not take.
// *aOriginal is null unless the hook was installed.
bool TryAttach(uintptr_t aTarget, void* aDetour, void** aOriginal, const char* aName);

}
