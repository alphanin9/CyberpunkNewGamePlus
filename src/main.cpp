#include <Windows.h>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <ArchiveXL.hpp>
#include <TweakXL.hpp>

#include <context/context.hpp>

#include <hooking/hooking.hpp>
#include <util/migration/migration.hpp>

#include <config/projectTemplate.hpp>

#include <Shared/Raw/Assert/AssertionFailed.hpp>

using namespace Red;

static constexpr auto c_archiveName = "NewGamePlus.archive";
static constexpr auto c_tweaksFolder = "tweaks";

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(PluginHandle aHandle, EMainReason aReason, const Sdk* aSdk)
{
    switch (aReason)
    {
    case EMainReason::Load:
    {
        PluginContext::m_redPlugin = aHandle;
        PluginContext::m_redSdk = aSdk;

        migration::RemoveUnusedFiles();

        TypeInfoRegistrar::RegisterDiscovered();

        // Create struct in shared data with necessary shims for ArchiveXL/TweakXL later
        // Or not, it might want recursion I think

        if (!aSdk->scripts->Add(aHandle, L"redscript\\"))
        {
            PluginContext::Error("Failed to add scripts, quitting...");
            return false;
        }

        if (!ArchiveXL::RegisterArchive(aHandle, c_archiveName))
        {
            PluginContext::Error("Failed to add archive, quitting...");
            return false;
        }

        if (!TweakXL::RegisterTweaks(aHandle, c_tweaksFolder))
        {
            PluginContext::Error("Failed to add tweaks, quitting...");
            return false;
        }

        if (!hooking::InitializeHooking())
        {
            return false;
        }

        PluginContext::m_rtti = CRTTISystem::Get();
        PluginContext::m_rtti->AddPostRegisterCallback(
            []()
            {
                PluginContext::m_rttiReady = true;
            });
        break;
    }
    case EMainReason::Unload:
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

RED4EXT_C_EXPORT void RED4EXT_CALL Query(PluginInfo* aInfo)
{
    aInfo->name = L"New Game+";
    aInfo->author = L"not_alphanine";
    aInfo->version = RED4EXT_SEMVER_EX(static_cast<std::uint8_t>(build::c_version.major),
                                       static_cast<std::uint8_t>(build::c_version.minor),
                                       static_cast<std::uint8_t>(build::c_version.patch),
                                       RED4EXT_V0_SEMVER_PRERELEASE_TYPE_NONE, 0); // Set your version here.
    aInfo->runtime = RED4EXT_RUNTIME_INDEPENDENT;
    aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_LATEST;
}

// Note: added to grab module ptr for migration to get NG+ path
BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReason, LPVOID)
{
    if (aReason == DLL_PROCESS_ATTACH)
    {
        migration::SetupModulePath(aModule);
    }

    return TRUE;
}