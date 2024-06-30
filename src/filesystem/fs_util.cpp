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

    // NOTE: CP2077 doesn't actually seem to use FOLDERID_SavedGames - does this current method play nice with other language versions of Windows?
    SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_CREATE, nullptr, &userProfilePath);
    if (!userProfilePath)
    {
        return std::filesystem::path{};
    }

    static const std::filesystem::path basePath = userProfilePath;
    static const std::filesystem::path finalPath = basePath / "Saved Games" / "CD Projekt Red" / "Cyberpunk 2077";

    return finalPath;
}

constexpr auto minSupportedGameVersion = 2000;

// No longer checks for PONR, needs a name change
bool HasValidPointOfNoReturnSave()
{
    static const auto basePath = GetCpSaveFolder();

    std::filesystem::directory_iterator directoryIterator{basePath};

    auto dirBegin = std::filesystem::begin(directoryIterator);
    auto dirEnd = std::filesystem::end(directoryIterator);

    return std::any_of(dirBegin, dirEnd,
                       [](const std::filesystem::directory_entry& aDirEntry)
                       {
                           if (!aDirEntry.is_directory())
                           {
                               return false;
                           }

                           const auto saveName = aDirEntry.path().stem().string();

                           return IsValidForNewGamePlus(saveName);
                       });
}

bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash) noexcept
{
    static const auto basePath = GetCpSaveFolder();

    constexpr auto shouldSpewSavePathForGoldy = false;

    if constexpr (shouldSpewSavePathForGoldy)
    {
        PluginContext::Spew(std::format("Processing save {}...", (basePath / aSaveName / "metadata.9.json").string()));
    }

    if (aSaveName.starts_with("EndGameSave"))
    {
        // Hack, not sure if necessary...
        return false;
    }

    const auto metadataPath = basePath / aSaveName / "metadata.9.json";

    std::error_code ec{};

    if (!std::filesystem::is_regular_file(metadataPath, ec))
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Metadata is not regular file!");
        }
        return false;
    }

    auto padded = simdjson::padded_string::load(metadataPath.string());

    if (padded.error() != simdjson::SUCCESS)
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Failed to load metadata into padded string!");
        }
        return false;
    }

    simdjson::dom::parser parser{};
    simdjson::dom::element document{};

    if (parser.parse(padded.value()).get(document) != simdjson::SUCCESS)
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Failed to parse metadata!");
        }
        return false;
    }

    if (!document.is_object())
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Metadata is not object!");
        }
        return false;
    }

    if (!document.at_key("RootType").is_string())
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Missing RootType!");
        }
        return false;
    }
    
    simdjson::dom::element saveMetadata{};

    if (document.at_key("Data").at_key("metadata").get(saveMetadata) != simdjson::SUCCESS)
    {
        if constexpr (shouldSpewSavePathForGoldy)
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
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Bad game version!");
        }
        return false;
    }

    if (minSupportedGameVersion > gameVersion)
    {
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Save is too old!");
        }
        return false;
    }

    const auto isPointOfNoReturn = aSaveName.starts_with("PointOfNoReturn");

    std::string_view playthroughId{};

    if (saveMetadata.at_key("playthroughID").get_string().get(playthroughId) != simdjson::SUCCESS)
    {
        if constexpr (shouldSpewSavePathForGoldy)
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
        if constexpr (shouldSpewSavePathForGoldy)
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
        if constexpr (shouldSpewSavePathForGoldy)
        {
            PluginContext::Spew("Necessary quests done, NG+ can be done!");
        }
        return true;
    }

    constexpr auto q307ActiveFact = "q307_blueprint_acquired=1";

    simdjson::dom::array importantFacts{};

    if (!saveMetadata.at_key("facts").get_array().get(importantFacts) != simdjson::SUCCESS)
    {
        if constexpr (shouldSpewSavePathForGoldy)
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
            return true;
        }
    }

    if constexpr (shouldSpewSavePathForGoldy)
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
