#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace files
{
inline constexpr auto c_metadataFileName = "metadata.9.json";
inline constexpr auto c_saveFileName = "sav.dat";

bool HasValidPointOfNoReturnSave() noexcept;

Red::CString GetRedPathToSaveFile(const char* aSaveName, const char* aFileName) noexcept;

bool IsValidForNewGamePlus(const Red::CString& aSaveFullPath) noexcept;
bool IsValidForNewGamePlus(const Red::CString& aSaveFullPath, uint64_t& aPlaythroughHash) noexcept;

bool ReadSaveFileToBuffer(const Red::CString& aSaveName, std::vector<std::byte>& aBuffer) noexcept;
} // namespace files