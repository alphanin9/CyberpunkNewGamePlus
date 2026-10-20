// macOS: the game's metadata loader (LoadSaveMetadataFromFile) has no entry in the Mac binary, so New Game Plus reads
// a save's metadata.9.json itself. Only the fields New Game Plus uses are filled; everything else keeps its default.

#include "NgpFeatures.hpp"

#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <RED4ext/Scripting/Natives/Generated/save/Metadata.hpp>

namespace
{
void ReadString(const nlohmann::json& aObject, const char* aKey, RED4ext::CString& aOut)
{
    auto it = aObject.find(aKey);
    if (it != aObject.end() && it->is_string())
    {
        aOut = RED4ext::CString(it->get_ref<const std::string&>().c_str());
    }
}

template<typename T>
void ReadNumber(const nlohmann::json& aObject, const char* aKey, T& aOut)
{
    auto it = aObject.find(aKey);
    if (it != aObject.end() && it->is_number())
    {
        aOut = it->get<T>();
    }
}

void ReadBool(const nlohmann::json& aObject, const char* aKey, bool& aOut)
{
    auto it = aObject.find(aKey);
    if (it != aObject.end() && it->is_boolean())
    {
        aOut = it->get<bool>();
    }
}
} // namespace

namespace ngp::mac
{
bool ReadSaveMetadataJson(const char* aPath, RED4ext::save::Metadata& aMetadata) noexcept
{
    try
    {
        std::ifstream file(aPath, std::ios::binary);
        if (!file)
        {
            return false;
        }

        const auto root = nlohmann::json::parse(file, nullptr, false);
        if (root.is_discarded() || !root.is_object())
        {
            std::fprintf(stderr, "[ngplus] metadata is not valid JSON: %s\n", aPath);
            return false;
        }

        const auto data = root.find("Data");
        if (data == root.end() || !data->is_object())
        {
            return false;
        }

        const auto metadata = data->find("metadata");
        if (metadata == data->end() || !metadata->is_object())
        {
            return false;
        }

        ReadString(*metadata, "gameDefinition", aMetadata.gameDefinition);
        ReadString(*metadata, "trackedQuestEntry", aMetadata.trackedQuestEntry);
        ReadString(*metadata, "debugString", aMetadata.debugString);
        ReadString(*metadata, "finishedQuests", aMetadata.finishedQuests);
        ReadString(*metadata, "playthroughID", aMetadata.playthroughID);
        ReadString(*metadata, "platform", aMetadata.platform);
        ReadNumber(*metadata, "saveVersion", aMetadata.saveVersion);
        ReadNumber(*metadata, "gameVersion", aMetadata.gameVersion);
        ReadBool(*metadata, "isPointOfNoReturn", aMetadata.isPointOfNoReturn);
        ReadBool(*metadata, "isEndGameSave", aMetadata.isEndGameSave);

        const auto facts = metadata->find("facts");
        if (facts != metadata->end() && facts->is_array())
        {
            for (const auto& fact : *facts)
            {
                if (fact.is_string())
                {
                    aMetadata.facts.PushBack(RED4ext::CString(fact.get_ref<const std::string&>().c_str()));
                }
            }
        }

        return true;
    }
    catch (...)
    {
        std::fprintf(stderr, "[ngplus] could not read metadata: %s\n", aPath);
        return false;
    }
}
} // namespace ngp::mac
