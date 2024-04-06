#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#include "filesystem/fs_util.hpp"
#include "parsing/fileReader.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

/* void runParsing() {
	const auto cpRootSavePath = files::getCpSaveFolder();
	const auto pointOfNoReturnSavePath = files::findLastPointOfNoReturnSave(cpRootSavePath);

	std::wcout << pointOfNoReturnSavePath.native() << std::endl;

	const auto savePath = pointOfNoReturnSavePath / L"sav.dat";
	const auto metadataPath = pointOfNoReturnSavePath / L"metadata.9.json";

	std::wcout << savePath.native() << std::endl;
	std::println("Size: {}", std::filesystem::file_size(savePath));

	parser::Parser fileParser;

	const auto ret = fileParser.parseSavegame(savePath);
	const auto ret2 = fileParser.parseMetadata(metadataPath);
} */

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk)
{
	switch (aReason)
	{
	case RED4ext::EMainReason::Load:
	{
		/*auto parserState = RED4ext::GameState{};

		parserState.OnEnter = nullptr;
		parserState.OnUpdate = [](RED4ext::CGameApplication* aApp) {
			runParsing();

			return true;
		};
		parserState.OnExit = nullptr;

		aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &parserState);*/
		Red::TypeInfoRegistrar::RegisterDiscovered();

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
	aInfo->name = L"Save parsing without loading save";
	aInfo->author = L"not_alphanine";
	aInfo->version = RED4EXT_SEMVER(0, 0, 0); // Set your version here.
	aInfo->runtime = RED4EXT_RUNTIME_LATEST;
	aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
	return RED4EXT_API_VERSION_LATEST;
}