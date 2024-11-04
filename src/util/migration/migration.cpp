#include <array>
#include <filesystem>
#include <string>

#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include <context/context.hpp>

#include "migration.hpp"

namespace migration
{
static std::filesystem::path m_modulePath{};
}

void migration::RemoveUnusedFiles()
{
    // To be updated with more later on...
    constexpr std::array c_unusedFiles = {
        L"redscript\\DifficultyAdjustment\\NGPlusDifficultyFixes.reds",
        L"redscript\\Scenario\\NGPlusStatsAdjustmentController.reds",
        L"redscript\\NGPlusEP1Listener.reds",
        L"redscript\\SpawnTagController.reds",
        L"redscript\\DifficultyAdjustment\\NGPlusDifficultyDefaultConfig.reds",
        L"tweaks\\NGPlus_BasegameFlatPatches.yaml",
    };

    auto cleanedUpAny = false;
    auto cleanupFailed = false;

    for (auto relativePath : c_unusedFiles)
    {
        auto pathToFile = m_modulePath / relativePath;
        PluginContext::Spew("Cleaning up {}...", pathToFile.string());

        std::error_code ec{};

        if (std::filesystem::is_regular_file(pathToFile, ec))
        {
            if (!std::filesystem::remove(pathToFile, ec))
            {
                PluginContext::Error("Failed to clean up {}, error: {}", pathToFile.string(), ec.message());
                cleanupFailed = true;
            }
            else
            {
                PluginContext::Spew("Cleaned file up!");
            }
            cleanedUpAny = true;
        }
    }

    if (cleanedUpAny)
    {
        PluginContext::Spew("Cleaned up unused files...");
    }

    if (cleanupFailed)
    {
        PluginContext::Error("Some files were not cleaned! Check the log to get the reasons.");
    }
}

// Here to not pollute context.hpp too much
void migration::SetupModulePath(HMODULE aModule)
{
    std::wstring modulePath{};
    wil::GetModuleFileNameW(aModule, modulePath);

    m_modulePath = modulePath;
    m_modulePath = m_modulePath.parent_path(); // red4ext/plugins/NewGamePlus
}