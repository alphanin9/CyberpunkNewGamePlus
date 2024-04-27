#include <algorithm>
#include <filesystem>
#include <fstream>
#include <print>

#include "Windows.h"
#include "shlobj_core.h"

#include <nlohmann/json.hpp>

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

struct SaveData
{
    std::filesystem::file_time_type m_time;
    std::filesystem::path m_path;
};

std::filesystem::path GetLatestPointOfNoReturnSave()
{
    constexpr auto pointOfNoReturnPrefix = L"PointOfNoReturn";
    constexpr auto saveMetadataType = "saveMetadataContainer";
    constexpr auto minSupportedGameVersion = 2000;

    const auto basePath = GetCpSaveFolder();

    auto vecSavePaths = std::vector<SaveData>{};

    for (const auto& dirEntry : std::filesystem::directory_iterator{basePath})
    {
        if (!dirEntry.is_directory())
        {
            continue;
        }

        if (!dirEntry.path().stem().native().starts_with(pointOfNoReturnPrefix))
        {
            continue;
        }

        const auto metadataPath = dirEntry.path() / L"metadata.9.json";

        if (!std::filesystem::is_regular_file(metadataPath))
        {
            continue;
        }

        const auto json = nlohmann::json::parse(std::ifstream{metadataPath});

        if (json.empty())
        {
            continue;
        }

        if (json["RootType"].get<std::string_view>() != saveMetadataType)
        {
            continue;
        }

        const auto& saveMetadata = json["Data"]["metadata"];

        if (saveMetadata["gameVersion"].get<std::int64_t>() >= minSupportedGameVersion)
        {
            SaveData data{};
            data.m_path = dirEntry;
            data.m_time = std::filesystem::last_write_time(metadataPath);

            vecSavePaths.push_back(data);
        }
    }

    if (vecSavePaths.empty())
    {
        return std::filesystem::path{};
    }

    std::sort(vecSavePaths.begin(), vecSavePaths.end(),
              [](const SaveData& aLhs, const SaveData& aRhs)
              { return aLhs.m_time.time_since_epoch() > aRhs.m_time.time_since_epoch(); });

    return vecSavePaths.at(0).m_path;
}

constexpr auto pointOfNoReturnPrefix = L"PointOfNoReturn";
constexpr auto saveMetadataType = "saveMetadataContainer";
constexpr auto minSupportedGameVersion = 2000;

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

                           if (!aDirEntry.path().stem().native().starts_with(pointOfNoReturnPrefix))
                           {
                               return false;
                           }

                           const auto metadataPath = aDirEntry.path() / L"metadata.9.json";

                           if (!std::filesystem::is_regular_file(metadataPath))
                           {
                               return false;
                           }

                           const auto json = nlohmann::json::parse(std::ifstream{metadataPath});

                           if (json.empty())
                           {
                               return false;
                           }

                           if (json["RootType"].get<std::string_view>() != saveMetadataType)
                           {
                               return false;
                           }

                           const auto& saveMetadata = json["Data"]["metadata"];

                           return saveMetadata["gameVersion"].get<std::int64_t>() >= minSupportedGameVersion;
                       });
}

bool IsValidForNewGamePlus(std::string_view aSaveName)
{
    static const auto basePath = GetCpSaveFolder();

    const auto metadataPath = basePath / aSaveName / "metadata.9.json";

    if (!std::filesystem::is_regular_file(metadataPath))
    {
        return false;
    }

    const auto json = nlohmann::json::parse(std::ifstream{metadataPath});

    if (json.empty())
    {
        return false;
    }

    if (json["RootType"].get<std::string_view>() != saveMetadataType)
    {
        return false;
    }

    const auto& saveMetadata = json["Data"]["metadata"];

    return saveMetadata["gameVersion"].get<std::int64_t>() >= minSupportedGameVersion;
}

} // namespace files
