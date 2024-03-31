#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "shlobj_core.h"
#include "Windows.h"


namespace files {
	std::filesystem::path getCpSaveFolder() {
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

	std::filesystem::path findLastPointOfNoReturnSave(std::filesystem::path cpSaveFolder) {
		auto lastPointOfNoReturnSave = std::filesystem::path{};

		for (const auto& dirEntry : std::filesystem::directory_iterator{ cpSaveFolder }) {
			if (dirEntry.is_directory() && dirEntry.path().stem().native().starts_with(L"PointOfNoReturnSave")) {
				lastPointOfNoReturnSave = dirEntry.path();
			}
		}

		return lastPointOfNoReturnSave;
	}
}
