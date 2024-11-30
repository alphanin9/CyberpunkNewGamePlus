#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/save/MetadataContainer.hpp>

namespace files
{
inline constexpr auto c_metadataFileName = "metadata.9.json";
inline constexpr auto c_saveFileName = "sav.dat";

bool HasValidPointOfNoReturnSave() noexcept;
void HasNewGamePlusSaveAsync(Red::WeakHandle<Red::IScriptable> aTarget, Red::CName aCallback) noexcept;

Red::CString GetRedPathToSaveFile(const char* aSaveName, const char* aFileName) noexcept;

bool LoadSaveMetadata(const Red::CString& aFilePath, Red::save::Metadata& aMetadataObject) noexcept;

bool IsValidForNewGamePlus(const Red::CString& aSaveFullPath) noexcept;
bool IsValidForNewGamePlus(const Red::CString& aSaveFullPath, uint64_t& aPlaythroughHash) noexcept;

bool ReadSaveFileToBuffer(const Red::CString& aSaveName, std::vector<std::byte>& aBuffer) noexcept;
} // namespace files