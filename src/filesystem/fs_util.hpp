#pragma once
#include <filesystem>
namespace files {
	std::filesystem::path GetCpSaveFolder();
	std::filesystem::path GetLatestPointOfNoReturnSave();
	bool HasValidPointOfNoReturnSave();
}