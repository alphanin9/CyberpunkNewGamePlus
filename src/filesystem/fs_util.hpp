#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <filesystem>
namespace files
{
inline constexpr const char* c_metadataFileName = "metadata.9.json";
inline constexpr const char* c_saveFileName = "sav.dat";

bool HasValidPointOfNoReturnSave() noexcept;

Red::CString GetRedPathToSaveFile(const char* aSaveName, const char* aFileName) noexcept;

bool IsValidForNewGamePlus(const Red::CString& aSaveName) noexcept;
bool IsValidForNewGamePlus(const Red::CString& aSaveName, uint64_t& aPlaythroughHash) noexcept;

bool ReadSaveFileToBuffer(const Red::CString& aSaveName, std::vector<std::byte>& aBuffer) noexcept;
} // namespace files