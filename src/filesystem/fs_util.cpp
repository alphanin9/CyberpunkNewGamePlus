#include <algorithm>
#include <filesystem>
#include <fstream>
#include <print>

#include "shlobj_core.h"
#include "Windows.h"

#include <nlohmann/json.hpp>

namespace files {
	std::filesystem::path GetCpSaveFolder() {
		PWSTR savedGamesPath = nullptr;

		const auto getSavedGamesResult = SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesPath);

		if (!savedGamesPath) {
			return std::filesystem::path{};
		}

		auto path = std::filesystem::path{ savedGamesPath };

		path /= L"CD Projekt Red";
		path /= L"Cyberpunk 2077";

		return path;
	}

	struct SaveData {
		std::filesystem::file_time_type m_time;
		std::filesystem::path m_path;
	};

	std::filesystem::path GetLatestPointOfNoReturnSave() {
		constexpr auto pointOfNoReturnPrefix = L"PointOfNoReturn";
		constexpr auto saveMetadataType = "saveMetadataContainer";
		constexpr auto minSupportedGameVersion = 2000;

		const auto basePath = GetCpSaveFolder();

		auto vecSavePaths = std::vector<SaveData>{};

		for (const auto& dirEntry : std::filesystem::directory_iterator{ basePath }) {
			if (!dirEntry.is_directory()) {
				continue;
			}

			if (!dirEntry.path().stem().native().starts_with(pointOfNoReturnPrefix)) {
				continue;
			}

			const auto metadataPath = dirEntry.path() / L"metadata.9.json";

			if (!std::filesystem::is_regular_file(metadataPath)) {
				continue;
			}

			const auto json = nlohmann::json::parse(std::ifstream{ metadataPath });

			if (json.empty()) {
				continue;
			}

			if (json["RootType"].get<std::string_view>() != saveMetadataType) {
				continue;
			}

			const auto& saveMetadata = json["Data"]["metadata"];

			if (saveMetadata["gameVersion"].get<std::int64_t>() >= minSupportedGameVersion) {
				SaveData data{};
				data.m_path = dirEntry;
				data.m_time = std::filesystem::last_write_time(metadataPath);

				vecSavePaths.push_back(data);
			}
		}

		if (vecSavePaths.empty()) {
			return std::filesystem::path{};
		}

		std::sort(vecSavePaths.begin(), vecSavePaths.end(), [](const SaveData& aLhs, const SaveData& aRhs) {
			return aLhs.m_time.time_since_epoch() > aRhs.m_time.time_since_epoch();
		});

		constexpr auto printLoadingFile = false;

		if constexpr (printLoadingFile) {
			std::println("Loading file {}...", vecSavePaths.at(0).m_path.string());
		}

		return vecSavePaths.at(0).m_path;
	}
}
