#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <unordered_set>

#include "Windows.h"
#include "shlobj_core.h"

#include <simdjson.h>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "fs_util.hpp"

#include "../context/context.hpp"

namespace files
{
std::filesystem::path GetCpSaveFolder()
{
    PWSTR userProfilePath{};

    // NOTE: CP2077 doesn't actually seem to use FOLDERID_SavedGames - does current method play nice with other language
    // versions of Windows?
    SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_CREATE, nullptr, &userProfilePath);
    if (!userProfilePath)
    {
        PluginContext::Error("FOLDERID_Profile could not be found!");
        return std::filesystem::path{};
    }

    static const std::filesystem::path basePath = userProfilePath;
    static const std::filesystem::path finalPath = basePath / L"Saved Games" / L"CD Projekt Red" / L"Cyberpunk 2077";

    return finalPath;
}

// Too lazy to bother with anything older LOL
constexpr std::int64_t minSupportedGameVersion = 2000;

// No longer checks for PONR, needs a name change
bool HasValidPointOfNoReturnSave()
{
    static const auto basePath = GetCpSaveFolder();

    std::filesystem::directory_iterator directoryIterator{basePath};

    for (auto& i : directoryIterator)
    {
        if (!i.is_directory())
        {
            continue;
        }

        const auto saveName = i.path().stem().string();

        if (IsValidForNewGamePlus(saveName))
        {
            return true;
        }
    }

    return false;
}

bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash) noexcept
{
    static const auto basePath = GetCpSaveFolder();

    constexpr auto shouldDebugMetadataValidation = false;

    if (aSaveName.starts_with("EndGameSave"))
    {
        // Hack, not sure if necessary...
        return false;
    }

    const auto metadataPath = basePath / aSaveName / "metadata.9.json";

    if constexpr (shouldDebugMetadataValidation)
    {
        PluginContext::Spew(std::format("Processing save {}...", metadataPath.string()));
    }

    std::error_code ec{};

    if (!std::filesystem::is_regular_file(metadataPath, ec))
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Metadata is not regular file!");
        }
        return false;
    }

    auto padded = simdjson::padded_string::load(metadataPath.string());

    if (padded.error() != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Failed to load metadata into padded string!");
        }
        return false;
    }

    static simdjson::dom::parser parser{};
    simdjson::dom::element document{};

    // NOTE: some users have this fail... Due to arch check, should setting arch to default fix it?
    if (const auto errorCode = parser.parse(padded.value()).get(document); errorCode != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Failed to parse metadata! Error {}", simdjson::error_message(errorCode));
        }
        return false;
    }

    if (!document.is_object())
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Metadata is not object!");
        }
        return false;
    }

    if (!document.at_key("RootType").is_string())
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Missing RootType!");
        }
        return false;
    }

    simdjson::dom::element saveMetadata{};

    if (document.at_key("Data").at_key("metadata").get(saveMetadata) != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Inner metadata not found!");
        }
        return false;
    }

    // ISSUE: old saves (might not be from PC?) don't have save metadata correct
    // Switch to noexcept versions

    int64_t gameVersion{};

    if (saveMetadata.at_key("gameVersion").get_int64().get(gameVersion) != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Bad game version!");
        }
        return false;
    }

    if (minSupportedGameVersion > gameVersion)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew(std::format("Save is too old, minimum supported game version: {}, save game version: {}", minSupportedGameVersion, gameVersion));
        }
        return false;
    }

    const auto isPointOfNoReturn = aSaveName.starts_with("PointOfNoReturn");

    std::string_view playthroughId{};

    if (saveMetadata.at_key("playthroughID").get_string().get(playthroughId) != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("playthroughID not found!");
        }
        return false;
    }

    aPlaythroughHash = Red::FNV1a64(playthroughId.data());

    if (isPointOfNoReturn)
    {
        return true;
    }

    std::string_view questsDone{};

    if (saveMetadata.at_key("finishedQuests").get_string().get(questsDone) != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("finishedQuests not found!");
        }
        return false;
    }

    using std::operator""sv;
    auto questsSplitRange = std::views::split(questsDone, " "sv);

    std::unordered_set<std::string_view> questsSet(questsSplitRange.begin(), questsSplitRange.end());

    if (questsSet.contains("q104") && questsSet.contains("q110") && questsSet.contains("q112"))
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Necessary quests done, NG+ can be done!");
        }
        return true;
    }

    constexpr auto q307ActiveFact = "q307_blueprint_acquired=1";

    simdjson::dom::array importantFacts{};

    // Oops, forgot to remove a ! here...
    if (saveMetadata.at_key("facts").get_array().get(importantFacts) != simdjson::SUCCESS)
    {
        if constexpr (shouldDebugMetadataValidation)
        {
            PluginContext::Spew("Important facts not found!");
        }
        return false;
    }

    for (const auto& fact : importantFacts)
    {
        std::string_view factValueString{};

        if (fact.get_string().get(factValueString) != simdjson::SUCCESS)
        {
            continue;
        }

        if (factValueString == q307ActiveFact)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                PluginContext::Spew("Save has Q307 available, good!");
            }
            return true;
        }
    }

    if constexpr (shouldDebugMetadataValidation)
    {
        PluginContext::Spew("Save does not meet criteria!");
    }
    return false;
}

bool IsValidForNewGamePlus(std::string_view aSaveName) noexcept
{
    uint64_t dummy{};
    return IsValidForNewGamePlus(aSaveName, dummy);
}
} // namespace files
