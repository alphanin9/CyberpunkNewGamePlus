#pragma once
#include <filesystem>
namespace files {
	std::filesystem::path GetCpSaveFolder();
	bool HasValidPointOfNoReturnSave();
    bool IsValidForNewGamePlus(std::string_view aSaveName) noexcept;
    bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash) noexcept;
}