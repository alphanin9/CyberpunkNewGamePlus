#include <chrono>
#include <unordered_set>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include <RED4ext/Scripting/Natives/entIPlacedComponent.hpp>

#include <context/context.hpp>

#include <filesystem/filesystem.hpp>
#include <parsing/fileReader.hpp>

#include <util/settings/settingsAccessor.hpp>

#include "definitions/playerSaveData.hpp"

#include <raw/playerSystem.hpp>
#include <raw/questsSystem.hpp>
#include <raw/save.hpp>
#include <raw/world.hpp>

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

enum class ENewGamePlusSaveVersion : std::uint32_t
{
    Initial = 1u
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

    static constexpr ResourcePath c_ngPlusQuest = R"(mod\quest\newgameplus.quest)";
    static constexpr ResourcePath c_ngPlusPrologueQuest = R"(mod\quest\newgameplus_q001.quest)";
    static constexpr ResourcePath c_ngPlusStandaloneQuest = R"(mod\quest\newgameplus_standalone.quest)";

    bool IsInNewGamePlusSave()
    {
        return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist || m_isInNewGamePlusStandalone;
    }

    // Only checks for NG+ and NG+ Q001 quests, not standalone
    bool IsInNonStandaloneNewGamePlusSave()
    {
        return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist;
    }

    void UpdateNewGamePlusState()
    {
        if (!m_questsSystem)
        {
            return;
        }

        // https://github.com/psiberx/cp2077-archive-xl/blob/9653e9d2eb07831941533fdff3839fc9bef80c8d/src/Red/QuestsSystem.hpp#L169
        // Should be accessed somewhere in QuestsSystem::OnGameLoad, I think?
        auto& questsList = raw::QuestsSystem::QuestsList::Ref(m_questsSystem);

        // We can't have multiple NG+ quests in the same game, so returning immediately is fine
        for (auto questResource : questsList)
        {
            if (questResource == c_ngPlusQuest)
            {
                m_isInNewGamePlusHeist = true;
                return;
            }
            else if (questResource == c_ngPlusPrologueQuest)
            {
                m_isInNewGamePlusPrologue = true;
                return;
            }
            else if (questResource == c_ngPlusStandaloneQuest)
            {
                m_isInNewGamePlusStandalone = true;
                return;
            }
        }
    }

    

    void LoadExpansionIntoSave()
    {
        // Vtable index 62
        // Sig: 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 ED B8

        if (!m_questsSystem)
        {
            PluginContext::Error("[LoadExpansionIntoSave] m_questsSystem == NULL");
            return;
        }

        constexpr ResourcePath EP1 = R"(ep1\quest\ep1.quest)";

        raw::QuestsSystem::AddQuest(m_questsSystem, EP1);
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

        // Note: I believe EP1 and non-EP1 resolving should take place here, not script-side
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

#pragma region InteriorDetection
    void TickInteriors()
    {
        // Rebuilt IsEntityInInteriorArea
        if (!m_playerSystem || !m_questsSystem)
        {
            m_isExterior = false;
            return;
        }

        if (!m_modConfig.m_enableRandomEncounters || !m_modConfig.m_useExteriorDetectionForRandomEncounters ||
            !m_isInNewGamePlusSave)
        {
            m_isExterior = false;
            return;
        }

        Handle<game::Object> player{};
        raw::cp::PlayerSystem::GetPlayerControlledGameObject(m_playerSystem, player);

        if (!player || player->status != EntityStatus::Attached)
        {
            m_isExterior = false;
            return;
        }

        auto runtimeScene = player->runtimeScene;

        // *(_QWORD *)(*(_QWORD *)(*(_QWORD *)(entityPtr + 0xB8) + 8LL) + 112LL) )
        auto unk1 = util::OffsetPtr<0x8, void*>::Ref(runtimeScene);
        auto unk2 = util::OffsetPtr<0x70, void*>::Ref(unk1);

        if (!unk2)
        {
            m_isExterior = false;
            return;
        }
        
        auto& coordinates = player->transformComponent->worldTransform.Position;
        int result{};

        m_isExterior = *raw::World::IsInInterior(unk2, &result, coordinates, false) == 0;
    }

    void TickFactsDB()
    {
        if (m_questsSystem)
        {
            raw::QuestsSystem::FactsDB(m_questsSystem)->SetFact("ngplus_is_outside", static_cast<int>(m_isExterior));
        }
    }
#pragma endregion

#pragma region Overrides
    void OnTick(FrameInfo& aFrame, JobQueue& aJob)
    {
        TickInteriors();
        TickFactsDB();
    }

    void OnRegisterUpdates(UpdateRegistrar* aRegistrar) override
    {
        aRegistrar->RegisterUpdate(UpdateTickGroup::EntityUpdateState, this, "NewGamePlusSystem/Tick",
                                   [this](FrameInfo& aFrame, JobQueue& aJob) { this->OnTick(aFrame, aJob); });
    }

    void OnGameLoad(const JobGroup& aJobGroup, bool& aSuccess, void* aStream) override
    {
        auto stream = static_cast<BaseStream*>(aStream);

        static auto s_nodeName = CNamePool::Add("NewGamePlusSystem");
        
        raw::Save::NodeAccessor node(stream, s_nodeName, false, false);

        if (!node.m_nodeIsPresentInSave)
        {
            // Not NG+ save, skip it
            return;
        }

        ENewGamePlusSaveVersion version{};

        stream->ReadWriteEx(&version);

        if (version >= ENewGamePlusSaveVersion::Initial)
        {
            stream->ReadWriteEx(&m_isInNewGamePlusPrologue);
            stream->ReadWriteEx(&m_isInNewGamePlusHeist);
            stream->ReadWriteEx(&m_isInNewGamePlusStandalone);
        }

        m_restoredDataFromSave = true;
    }

    std::uint32_t OnBeforeGameSave(const JobGroup& aJobGroup, void* aMetadataObject) override
    {
        // Unfortunately we can't add more metadata from mod, would be very useful
        return static_cast<std::uint32_t>(IsInNewGamePlusSave()); // We only need to do anything when NG+ is active
    }

    void OnGameSave(void* aStream) override
    {
        auto stream = static_cast<BaseStream*>(aStream);

        static auto s_nodeName = CNamePool::Add("NewGamePlusSystem");

        raw::Save::NodeAccessor node(stream, s_nodeName, true, false);

        auto version = ENewGamePlusSaveVersion::Initial;

        stream->ReadWriteEx(&version);
        stream->ReadWriteEx(&m_isInNewGamePlusPrologue);
        stream->ReadWriteEx(&m_isInNewGamePlusHeist);
        stream->ReadWriteEx(&m_isInNewGamePlusStandalone);
    }

    void OnGamePrepared() override
    {
        m_questsSystem = GetGameSystem<quest::QuestsSystem>();
        m_playerSystem = GetGameSystem<cp::PlayerSystem>();

        m_modConfig = settings::GetRandomEncounterSettings();
        // If we haven't restored, update NG+ config
        if (!m_restoredDataFromSave)
        {
            UpdateNewGamePlusState();
        }

        m_isInNewGamePlusSave = IsInNonStandaloneNewGamePlusSave();

        if constexpr (c_forceOutdoorsSpawns)
        {
            m_isInNewGamePlusSave = true;
        }
    }

    void OnWorldDetached(world::RuntimeScene* aScene) override
    {
        // Clean up state

        m_playerSystem = nullptr;
        m_questsSystem = nullptr;

        m_isInNewGamePlusSave = false;

        m_isInNewGamePlusPrologue = false;
        m_isInNewGamePlusHeist = false;
        m_isInNewGamePlusStandalone = false;

        m_isExterior = false;
        m_restoredDataFromSave = false;
    }
#pragma endregion
private:
    Handle<NGPlusProgressionData> m_progressionData{};

    settings::ModConfig m_modConfig{};

    bool m_isExterior{};
    bool m_restoredDataFromSave{};

    bool m_isInNewGamePlusPrologue{};
    bool m_isInNewGamePlusHeist{};
    bool m_isInNewGamePlusStandalone{};

    bool m_isInNewGamePlusSave{};
    bool m_isInStandalone{};

    cp::PlayerSystem* m_playerSystem{};
    quest::QuestsSystem* m_questsSystem{};

    static constexpr auto c_forceOutdoorsSpawns = true;

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