#pragma once
#include <filesystem>
namespace files {
	std::filesystem::path getCpSaveFolder();
	std::filesystem::path findLastPointOfNoReturnSave(std::filesystem::path cpSaveFolder);
}