#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#include "filesystem/fs_util.hpp"
#include "parsing/fileReader.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>
#include <ArchiveXL.hpp>

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk)
{
	switch (aReason)
	{
	case RED4ext::EMainReason::Load:
	{
		Red::TypeInfoRegistrar::RegisterDiscovered();

		// shhhhhhhhhhhhh
		// Nothing is happening here :)
		constexpr auto shouldLoadTopSecretArchive = false;
		if constexpr (shouldLoadTopSecretArchive) {
			// I wonder what this could contain!
			ArchiveXL::RegisterArchive(aHandle, L"TopSecret.archive"); // :)
		}

		break;
	}
	case RED4ext::EMainReason::Unload:
	{
		// Free memory, detach hooks.
		// The game's memory is already freed, to not try to do anything with it.
		break;
	}
	}

	return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
	aInfo->name = L"Top Secret Module";
	aInfo->author = L"not_alphanine";
	aInfo->version = RED4EXT_SEMVER(0, 0, 1); // Set your version here.
	aInfo->runtime = RED4EXT_RUNTIME_LATEST;
	aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
	return RED4EXT_API_VERSION_LATEST;
}