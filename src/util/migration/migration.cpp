#include <array>
#include <filesystem>
#include <string>

#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include <context.hpp>

#include "migration.hpp"

void migration::RemoveUnusedFiles()
{
    // To be updated with more later on...
    constexpr std::array c_unusedFiles = {
        L"red4ext\\plugins\\NewGamePlus\\redscript\\DifficultyAdjustment\\NGPlusDifficultyFixes.reds",
        L"red4ext\\plugins\\NewGamePlus\\redscript\\Scenario\\NGPlusStatsAdjustmentController.reds",
        L"red4ext\\plugins\\NewGamePlus\\tweaks\\NGPlus_BasegameFlatPatches.yaml",
        L"red4ext\\plugins\\NewGamePlus\\redscript\\NGPlusEP1Listener.reds",
        L"red4ext\\plugins\\NewGamePlus\\redscript\\SpawnTagController.reds"
        L"red4ext\\plugins\\NewGamePlus\\redscript\\DifficultyAdjustment\\NGPlusDifficultyDefaultConfig.reds"};

    std::wstring modulePath{};

    wil::GetModuleFileNameW(GetModuleHandleW(nullptr), modulePath);

    std::filesystem::path exePath = modulePath;
    // bin\\x64\\Cyberpunk2077.exe
    const auto rootDirectory = exePath.parent_path().parent_path().parent_path();

    auto cleanedUpAny = false;

    for (auto relativePath : c_unusedFiles)
    {
        auto pathToFile = rootDirectory / relativePath;

        std::error_code ec{};

        if (std::filesystem::is_regular_file(pathToFile, ec))
        {
            std::filesystem::remove(pathToFile, ec);
            cleanedUpAny = true;
        }
    }

    if (cleanedUpAny)
    {
        PluginContext::Spew("Cleaned up unused files...");
    }
}