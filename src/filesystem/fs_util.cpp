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

bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash)
{
    static const auto basePath = GetCpSaveFolder();

    if (aSaveName.starts_with("EndGameSave"))
    {
        // Hack, not sure if necessary...
        return false;
    }

    const auto metadataPath = basePath / aSaveName / "metadata.9.json";

    if (!std::filesystem::is_regular_file(metadataPath))
    {
        return false;
    }

    auto padded = simdjson::padded_string::load(metadataPath.string());

    if (padded.error() != simdjson::error_code::SUCCESS)
    {
        return false;
    }

    auto json2 = simdjson::dom::parser{};

    const auto document = json2.parse(padded.value());

    if (!document.is_object())
    {
        return false;
    }

    if (!document["RootType"].is_string())
    {
        return false;
    }

    const auto saveMetadata = document["Data"]["metadata"];

    if (minSupportedGameVersion > int64_t(saveMetadata["gameVersion"]))
    {
        return false;
    }

    const auto isPointOfNoReturn = aSaveName.starts_with("PointOfNoReturn");

    aPlaythroughHash = Red::FNV1a64(saveMetadata["playthroughID"].get_c_str().value());
    
    if (isPointOfNoReturn)
    {
        return true;
    }

    const auto questsDone = saveMetadata["finishedQuests"].get_string().value();

    using std::operator""sv;
    auto questsSplitRange = std::views::split(questsDone, " "sv);

    std::unordered_set<std::string_view> questsSet(questsSplitRange.begin(), questsSplitRange.end());
    
    if (questsSet.contains("q104") && questsSet.contains("q110") && questsSet.contains("q112"))
    {
        return true;
    }

    constexpr auto q307_active_fact = "q307_blueprint_acquired=1";

    for (auto fact : saveMetadata["facts"])
    {
        if (fact.is_string())
        {
            if (fact.get_string().value() == q307_active_fact)
            {
                return true;
            }
        }
    }

    return false;
}

bool IsValidForNewGamePlus(std::string_view aSaveName)
{
    uint64_t dummy{};
    return IsValidForNewGamePlus(aSaveName, dummy);
}
} // namespace files
