#include <Windows.h>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <ArchiveXL.hpp>
#include <TweakXL.hpp>

#include <context/context.hpp>

#include <hooking/hooking.hpp>
#include <util/migration/migration.hpp>

using namespace Red;

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

        constexpr auto c_loadDependenciesFromPluginFolder = true;
        if constexpr (c_loadDependenciesFromPluginFolder)
        {
            if (!ArchiveXL::RegisterArchive(aHandle, L"NewGamePlus.archive"))
            {
                PluginContext::Error("Failed to load archive from the plugin's folder, quitting...");
                return false;
            }

            // NOTE: this could be done better, accounting for version migration on manual installs...
            // Would make including some crap in mod folder pointless too
            if (!aSdk->scripts->Add(aHandle, L"redscript\\"))
            {
                PluginContext::Error("Failed to add scripts from the plugin's folder, quitting...");
                return false;
            }

            if (!TweakXL::RegisterTweaks(aHandle, "tweaks\\"))
            {
                PluginContext::Error("Failed to add tweaks from the plugin's folder, quitting...");
                return false;
            }
        }

        if (!hooking::InitializeHooking())
        {
            return false;
        }

        PluginContext::m_rtti = Red::CRTTISystem::Get();
        PluginContext::m_rtti->AddPostRegisterCallback(
            []()
            {
                PluginContext::m_rttiReady = true;
                PluginContext::RegisterPluginChannelNames();
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
    aInfo->version = RED4EXT_SEMVER_EX(1, 1, 1, RED4EXT_V0_SEMVER_PRERELEASE_TYPE_NONE, 0); // Set your version here.
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