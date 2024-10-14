#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include <RedLib.hpp>

#include <context.hpp>

#include "../filesystem/fs_util.hpp"
#include "../parsing/fileReader.hpp"

#include "../util/offsetPtr.hpp"
#include "../util/threads.hpp"

#include <chrono>
#include <unordered_set>

#include "definitions/playerSaveData.hpp"

using namespace Red;

namespace redscript
{

enum class ENewGamePlusStartType
{
    StartFromQ001,
    StartFromQ101,
    StartFromQ001_NoEP1,
    StartFromQ101_NoEP1,
    StartFromQ101_ProgressionBuild,
    StartFromQ101_ProgressionBuild_NoEP1,
    Count,
    Invalid
};

class NewGamePlusSystem : public IGameSystem
{
public:
    Handle<NGPlusProgressionData> GetProgressionData()
    {
        return m_progressionData;
    }

    bool GetNewGamePlusState() const
    {
        return PluginContext::m_isNewGamePlusActive;
    }

    void SetNewGamePlusState(bool aNewState)
    {
        PluginContext::m_isNewGamePlusActive = aNewState;
    }

    bool GetStandaloneState() const
    {
        return m_isInStandalone;
    }

    void SetStandaloneState(bool aNewState)
    {
        m_isInStandalone = aNewState;
    }

    bool IsInNewGamePlusSave()
    {
        auto questsSystem = GetGameSystem<quest::QuestsSystem>();

        if (!questsSystem)
        {
            return false;
        }

        // https://github.com/psiberx/cp2077-archive-xl/blob/9653e9d2eb07831941533fdff3839fc9bef80c8d/src/Red/QuestsSystem.hpp#L169
        // Should be accessed somewhere in QuestsSystem::OnGameLoad, I think?
        auto& questsList = util::OffsetPtr<0xA8, DynArray<ResourcePath>>::Ref(questsSystem);

        constexpr std::array c_ngPlusQuests = {ResourcePath(R"(mod\quest\NewGamePlus.quest)"),
                                               ResourcePath(R"(mod\quest\NewGamePlus_Standalone.quest)"),
                                               ResourcePath(R"(mod\quest\NewGamePlus_Q001.quest)")};

        for (auto questResource : questsList)
        {
            if (std::find(c_ngPlusQuests.begin(), c_ngPlusQuests.end(), questResource) != c_ngPlusQuests.end())
            {
                return true;
            }
        }

        return false;
    }

    void LoadExpansionIntoSave()
    {
        // We only have the one expansion... Thank God

        // TODO: move all hashes into a header?
        // Vtable index 62
        // Sig: 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 ED B8
        constexpr auto addQuestHash = 1617892594u;

        // Maybe check if EP1 is already running? Eh, the Redscript side already does...

        static const auto fnAddQuest =
            UniversalRelocFunc<void(__fastcall*)(quest::QuestsSystem * aQuestsSystem, ResourcePath aPath)>(
                addQuestHash);

        constexpr ResourcePath EP1 = R"(ep1\quest\ep1.quest)";

        fnAddQuest(GetGameSystem<quest::QuestsSystem>(), EP1);
    }

    void SetNewGamePlusGameDefinition(ENewGamePlusStartType aStartType)
    {
        constexpr auto q001Path = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001.gamedef");
        constexpr auto q101Path = ResourcePath::HashSanitized("mod/quest/NewGamePlus.gamedef");

        // These ones do not have EP1 enabled by default, but will have it enabled via custom quest starting
        constexpr auto q001PathNoEP1 = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001_NoEP1.gamedef");
        constexpr auto q101PathNoEP1 = ResourcePath::HashSanitized("mod/quest/NewGamePlus_NoEP1.gamedef");

        // Bare Post-Heist starts, applying EP1 Standalone progression builds...
        constexpr auto standalonePath = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Standalone.gamedef");
        constexpr auto standalonePathNoEP1 =
            ResourcePath::HashSanitized("mod/quest/NewGamePlus_Standalone_NoEP1.gamedef");

        switch (aStartType)
        {
        case ENewGamePlusStartType::StartFromQ001:
            PluginContext::m_ngPlusGameDefinitionHash = q001Path;
            break;
        case ENewGamePlusStartType::StartFromQ101:
            PluginContext::m_ngPlusGameDefinitionHash = q101Path; // Should fix it being in base/ sometime
            break;
        case ENewGamePlusStartType::StartFromQ001_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = q001PathNoEP1;
            break;
        case ENewGamePlusStartType::StartFromQ101_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = q101PathNoEP1;
            break;
        case ENewGamePlusStartType::StartFromQ101_ProgressionBuild:
            PluginContext::m_ngPlusGameDefinitionHash = standalonePath;
            break;
        case ENewGamePlusStartType::StartFromQ101_ProgressionBuild_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = standalonePathNoEP1;
            break;
        default:
            break;
        }
    }

    bool HasPointOfNoReturnSave()
    {
        return files::HasValidPointOfNoReturnSave();
    }

    bool IsSaveValidForNewGamePlus(Red::ScriptRef<CString>& aSaveName)
    {
        return files::IsValidForNewGamePlus(aSaveName->c_str());
    }

    DynArray<int> ResolveNewGamePlusSaves(Red::ScriptRef<DynArray<CString>>& aSaves)
    {
        if (!aSaves)
        {
            return {};
        }

        // We don't need sorting, the game already sorts the saves before passing them to Redscript
        std::unordered_set<std::uint64_t> playthroughIds{};

        DynArray<int> returnedData{};
        returnedData.Reserve(aSaves->size);
        // Technically a signed/unsigned mismatch, but I have doubts about people having 2 billion+ saves
        for (auto i = 0u; i < aSaves->size; i++)
        {
            // We need the save index for this...
            auto& saveName = aSaves->entries[i];

            auto savePath = files::GetRedPathToSaveFile(saveName.c_str(), files::c_metadataFileName);

            std::uint64_t hash{};

            if (!files::IsValidForNewGamePlus(savePath, hash))
            {
                continue;
            }

            if (playthroughIds.contains(hash))
            {
                continue;
            }

            playthroughIds.insert(hash);

            returnedData.PushBack(int(i));
        }

        return returnedData;
    }

    bool ParsePointOfNoReturnSaveData(Red::ScriptRef<CString>& aSaveName)
    {
        if (!aSaveName)
        {
            return false;
        }

        auto start = std::chrono::high_resolution_clock{}.now();

        parser::Parser parser{};

        if (!parser.ParseSavegame(*aSaveName))
        {
            return false;
        }

        if (m_progressionData)
        {
            // Invalidate already present data
            m_progressionData.Reset();
        }

        // Constructor also runs all the progression logic
        m_progressionData = MakeHandle<NGPlusProgressionData>(parser);
        m_progressionData->PostProcess();

        auto end = std::chrono::high_resolution_clock{}.now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        PluginContext::Spew(std::format("Time taken: {}", duration));

        return true;
    }

    // Some users don't have LogChannel defined :P
    void Spew(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Spew(aStr->c_str());
    }

    void Error(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Error(aStr->c_str());
    }

private:
    Handle<NGPlusProgressionData> m_progressionData{};

    bool m_isInStandalone{};

    RTTI_IMPL_TYPEINFO(NewGamePlusSystem);
    RTTI_IMPL_ALLOCATOR();
};

} // namespace redscript

RTTI_DEFINE_ENUM(redscript::ENewGamePlusStartType);

RTTI_DEFINE_CLASS(redscript::NewGamePlusSystem, {
    RTTI_METHOD(ParsePointOfNoReturnSaveData);
    RTTI_METHOD(HasPointOfNoReturnSave);
    RTTI_METHOD(ResolveNewGamePlusSaves);
    RTTI_METHOD(GetNewGamePlusState);
    RTTI_METHOD(SetNewGamePlusState);
    RTTI_METHOD(GetProgressionData);
    RTTI_METHOD(SetNewGamePlusGameDefinition);
    RTTI_METHOD(IsSaveValidForNewGamePlus);
    RTTI_METHOD(LoadExpansionIntoSave);
    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);
    RTTI_METHOD(GetStandaloneState);
    RTTI_METHOD(SetStandaloneState);
    RTTI_METHOD(IsInNewGamePlusSave);
});