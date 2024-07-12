#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#include "context/context.hpp"
#include "hooking/hooking.hpp"
#include "redscript_api/redscriptBindings.hpp" // HACK, maybe that's why RTTI shit wasn't working right?

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>
#include <ArchiveXL.hpp>
#include <TweakXL.hpp>

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk)
{
	switch (aReason)
	{
	case RED4ext::EMainReason::Load:
	{
		PluginContext::m_redPlugin = aHandle;
		PluginContext::m_redSdk = aSdk;

		Red::TypeInfoRegistrar::RegisterDiscovered();

		constexpr auto loadDependenciesFromPluginFolder = true;
		if constexpr (loadDependenciesFromPluginFolder) {
			if (!ArchiveXL::RegisterArchive(aHandle, L"NewGamePlus.archive")) {
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
		
		if (!hooking::InitializeHooking()) {
			return false;
		}

		PluginContext::m_rtti = Red::CRTTISystem::Get();
		break;
	}
	case RED4ext::EMainReason::Unload:
	{
		if (!hooking::DetachHooking()) {
			return false;
		}

		break;
	}
	}

	return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
	aInfo->name = L"New Game+";
	aInfo->author = L"not_alphanine";
    aInfo->version = RED4EXT_SEMVER_EX(1, 0, 3, RED4EXT_V0_SEMVER_PRERELEASE_TYPE_RC, 1); // Set your version here.
	aInfo->runtime = RED4EXT_RUNTIME_INDEPENDENT;
	aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
	return RED4EXT_API_VERSION_LATEST;
}