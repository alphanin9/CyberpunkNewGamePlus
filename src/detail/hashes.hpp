#pragma once
namespace detail::Hashes
{
#pragma region FileSystem
constexpr auto FileStream_dtor = 1924865980u;
constexpr auto FileSystem_GetSaveFolder = 4293661415u;

constexpr auto RedFileManager_Instance = 3788966949u;
constexpr auto RedFileManager_OpenFileStream = 1917464012u;
constexpr auto RedFileManager_FindFiles = 4051576167u;

constexpr auto FileSystem_MergeFileToPath = 1194333091u;
constexpr auto FileSystem_MergeDirToPath = 836113218u; // Does a few more checks for stuff
#pragma endregion

#pragma region PlayerSystem
constexpr auto PlayerSystem_GetPlayerControlledGameObject = 3420005096u;
#pragma endregion

#pragma region QuestsSystem
constexpr auto QuestsSystem_AddQuest = 1617892594u;
constexpr auto QuestsSystem_CreateContext = 3144298192u;
constexpr auto QuestPhaseInstance_ExecuteNode = 3227858325u;
#pragma endregion

#pragma region SaveMetadata
constexpr auto SaveMetadata_LoadSaveMetadataFromFile = 1649938065u;
#pragma endregion

#pragma region World
constexpr auto World_IsInInterior = 3384552122u;
#pragma endregion
}; // namespace detail::Hashes