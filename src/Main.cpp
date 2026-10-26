#ifndef __APPLE__
#include <Windows.h>
#endif

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#ifndef __APPLE__
#include <RED4ext/ArchiveXL.hpp>
#include <RED4ext/TweakXL.hpp>
#else
#include <RED4ext/ArchiveXL.hpp>
#include <RED4ext/TweakXL.hpp>
#include "MacShims.hpp"

#include <RED4ext/Mac/PluginSupport.hpp>
#include <atomic>
#endif

#include <Context/Context.hpp>

#include <Hooking/Hooking.hpp>
#include <Util/Migration/Migration.hpp>

#include <Config/ProjectTemplate.hpp>

#include <Shared/Raw/Assert/AssertionFailed.hpp>

using namespace Red;

#ifndef __APPLE__
static constexpr auto ScriptsFolder = L"redscript\\";
#endif
static constexpr auto ArchiveName = "NewGamePlus.archive";
static constexpr auto TweaksFolder = "tweaks";

#ifdef __APPLE__
static std::atomic<bool> s_ngpLoaded{false};

RED4EXT_C_EXPORT void RED4EXT_CALL ngplus_registerNatives()
{
    static std::atomic<bool> s_once{false};
    if (s_once.exchange(true))
    {
        NgpMac::Log("[ngplus] ngplus_registerNatives: already ran");
        return;
    }
    if (!s_ngpLoaded.load() || !NgpMac::PluginSdk())
    {
        NgpMac::Log("[ngplus] ngplus_registerNatives: Main(Load) did not finish");
        return;
    }
    if (!RED4ext::CRTTISystem::Get())
    {
        NgpMac::Log("[ngplus] ngplus_registerNatives: no RTTI system yet");
        return;
    }

    NgpMac::Log("[ngplus] ngplus_registerNatives vault self-test: %s", RED4ext::Mac::VaultSelfTest() ? "ok" : "FAILED");
    // The macOS loader calls this after the game's RTTI registration phase, so RedLib's queued registrations run here.
    NgpMac::Log("[ngplus] ngplus_registerNatives RedLib queued: %zu register, %zu describe",
                Red::TypeInfoRegistrar::PendingRegisterCount(), Red::TypeInfoRegistrar::PendingDescribeCount());
    Red::TypeInfoRegistrar::RunPending();

    // The game system is registered once the GameInstance exists (after the scripts load).
    NgpMac::ChainGameInstanceCtor();

    std::error_code archiveError;
    const auto macArchive = NgpMac::RootDir() / "archive" / "Mac" / "mod" / ArchiveName;
    if (std::filesystem::is_regular_file(macArchive, archiveError))
    {
        NgpMac::Log("[ngplus] archive is in archive/Mac/mod (ArchiveXL mounts it; RegisterArchive skipped)");
    }
    else
    {
        const bool registered = ArchiveXL::RegisterArchive(NgpMac::PluginHandleValue(), ArchiveName);
        NgpMac::Log("[ngplus] RegisterArchive %s", registered ? "ok" : "failed");
    }

    const bool tweaks = TweakXL::RegisterTweaks(NgpMac::PluginHandleValue(), TweaksFolder);
    NgpMac::Log("[ngplus] RegisterTweaks %s", tweaks ? "ok" : "failed");

    PluginContext::m_rtti = CRTTISystem::Get();
    PluginContext::m_rttiReady = true;
    NgpMac::Log("[ngplus] ngplus_registerNatives done (RedLib left %zu / %zu)", Red::TypeInfoRegistrar::PendingRegisterCount(),
                Red::TypeInfoRegistrar::PendingDescribeCount());
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        PluginContext::m_redPlugin = aHandle;
        PluginContext::m_redSdk = aSdk;

        NgpMac::SetPlugin(aSdk, aHandle);

        migration::RemoveUnusedFiles();

        NgpMac::Log("[ngplus] scripts->Add skipped; natives register from ngplus_registerNatives");

        if (!hooking::InitializeHooking())
        {
            return false;
        }

        s_ngpLoaded.store(true);
        // The macOS loader calls Main before the game's RTTI system exists (CRTTISystem::Get() would construct it here and
        // crash), and calls ngplus_registerNatives itself once the scripts are loaded.
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        if (!hooking::DetachHooking())
        {
            return false;
        }

        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = L"New Game+";
    aInfo->author = L"not_alphanine";

    constexpr auto ModVersion = build::GetModVersion();

    aInfo->version = RED4EXT_V1_SEMVER_EX(static_cast<std::uint8_t>(ModVersion.major()),
                                          static_cast<std::uint8_t>(ModVersion.minor()),
                                          static_cast<std::uint8_t>(ModVersion.patch()),
                                          RED4EXT_V1_SEMVER_PRERELEASE_TYPE_NONE, 0);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    // The macOS RED4ext loader accepts API version 0 only; the v1 plugin info is compatible with it.
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_1_0_0_COMPAT_0_5_0;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1_COMPAT_0;
}
#else
RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason, const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        PluginContext::m_redPlugin = aHandle;
        PluginContext::m_redSdk = aSdk;

        migration::RemoveUnusedFiles();

        TypeInfoRegistrar::RegisterDiscovered();

        // Create struct in shared data with necessary shims for ArchiveXL/TweakXL later
        // Or not, it might want recursion I think

        if (!aSdk->scripts->Add(aHandle, ScriptsFolder))
        {
            PluginContext::Error("Failed to add scripts, quitting...");
            return false;
        }

        if (!ArchiveXL::RegisterArchive(aHandle, ArchiveName))
        {
            PluginContext::Error("Failed to add archive, quitting...");
            return false;
        }

        if (!TweakXL::RegisterTweaks(aHandle, TweaksFolder))
        {
            PluginContext::Error("Failed to add tweaks, quitting...");
            return false;
        }

        if (!hooking::InitializeHooking())
        {
            return false;
        }

        PluginContext::m_rtti = CRTTISystem::Get();
        PluginContext::m_rtti->AddPostRegisterCallback([]() { PluginContext::m_rttiReady = true; });
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        if (!hooking::DetachHooking())
        {
            return false;
        }

        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = L"New Game+";
    aInfo->author = L"not_alphanine";

    constexpr auto ModVersion = build::GetModVersion();

    aInfo->version =
        RED4EXT_V1_SEMVER_EX(static_cast<std::uint8_t>(ModVersion.major()), static_cast<std::uint8_t>(ModVersion.minor()),
                          static_cast<std::uint8_t>(ModVersion.patch()), RED4EXT_V1_SEMVER_PRERELEASE_TYPE_NONE,
                          0); // Set your version here.
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}
#endif
