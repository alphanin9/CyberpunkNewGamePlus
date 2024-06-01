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

namespace files
{
std::filesystem::path GetCpSaveFolder()
{
    PWSTR savedGamesPath = nullptr;

    const auto getSavedGamesResult =
        SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesPath);

    if (!savedGamesPath)
    {
        return std::filesystem::path{};
    }

    auto path = std::filesystem::path{savedGamesPath};

    path /= L"CD Projekt Red";
    path /= L"Cyberpunk 2077";

    return path;
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

    if (aSaveName.starts_with("EndGameSave"))
    {
        // Hack, not sure if necessary...
        return false;
    }

    const auto metadataPath = basePath / aSaveName / "metadata.9.json";

    std::error_code ec{};

    if (!std::filesystem::is_regular_file(metadataPath, ec))
    {
        return false;
    }

    auto padded = simdjson::padded_string::load(metadataPath.string());

    if (padded.error() != simdjson::SUCCESS)
    {
        return false;
    }

    simdjson::dom::parser parser{};
    simdjson::dom::element document{};

    if (parser.parse(padded.value()).get(document) != simdjson::SUCCESS)
    {
        return false;
    }

    if (!document.is_object())
    {
        return false;
    }

    if (!document.at_key("RootType").is_string())
    {
        return false;
    }
    
    simdjson::dom::element saveMetadata{};

    if (document.at_key("Data").at_key("metadata").get(saveMetadata) != simdjson::SUCCESS)
    {
        return false;
    }

    // ISSUE: old saves (might not be from PC?) don't have save metadata correct
    // Switch to noexcept versions

    int64_t gameVersion{};

    if (saveMetadata.at_key("gameVersion").get_int64().get(gameVersion) != simdjson::SUCCESS)
    {
        return false;
    }

    if (minSupportedGameVersion > gameVersion)
    {
        return false;
    }

    const auto isPointOfNoReturn = aSaveName.starts_with("PointOfNoReturn");

    std::string_view playthroughId{};

    if (saveMetadata.at_key("playthroughID").get_string().get(playthroughId) != simdjson::SUCCESS)
    {
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
        return false;
    }

    using std::operator""sv;
    auto questsSplitRange = std::views::split(questsDone, " "sv);

    std::unordered_set<std::string_view> questsSet(questsSplitRange.begin(), questsSplitRange.end());
    
    if (questsSet.contains("q104") && questsSet.contains("q110") && questsSet.contains("q112"))
    {
        return true;
    }

    constexpr auto q307ActiveFact = "q307_blueprint_acquired=1";

    simdjson::dom::array importantFacts{};

    if (!saveMetadata.at_key("facts").get_array().get(importantFacts) != simdjson::SUCCESS)
    {
        return false;
    }

    for (auto fact : importantFacts)
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

    return false;
}

bool IsValidForNewGamePlus(std::string_view aSaveName) noexcept
{
    uint64_t dummy{};
    return IsValidForNewGamePlus(aSaveName, dummy);
}
} // namespace files
