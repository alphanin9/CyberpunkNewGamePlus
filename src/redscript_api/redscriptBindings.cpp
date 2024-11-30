#include <chrono>
#include <unordered_set>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/gsm/MainQuest.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>
#include <RED4ext/Scripting/Natives/gameGameSessionDesc.hpp>

#include <RED4ext/Scripting/Natives/entIPlacedComponent.hpp>

#include <context/context.hpp>

#include <filesystem/SaveFS.hpp>

#include <util/settings/settingsAccessor.hpp>

#include "definitions/playerSaveData.hpp"

#include <util/scopeguard.hpp>

#include <Shared/Raw/GameDefinition/GameDefinition.hpp>
#include <Shared/Raw/Ink/InkSystem.hpp>
#include <Shared/Raw/PlayerSystem/PlayerSystem.hpp>
#include <Shared/Raw/Quest/QuestsSystem.hpp>
#include <Shared/Raw/Save/Save.hpp>
#include <Shared/Raw/World/SafeAreaManager.hpp>
#include <Shared/Raw/World/World.hpp>
#include <Shared/Util/Core.hpp>
#include <Shared/Util/NamePoolRegistrar.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationState.hpp>

#include <parsing/fileReader.hpp>

#include <tsl/hopscotch_set.h>

using namespace Red;

namespace mod
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

// Not used yet, placeholder
enum class ENGPlusType
{
    StartFromQ001,
    StartFromQ101,
    StartFromQ101_ProgressionBuild,
    Count,
    Invalid
};

enum class ENewGamePlusSaveVersion : std::uint32_t
{
    Initial = 1u,
    RandomEncounterChickenTestInDetection = 2u
};

class NewGamePlusSystem : public IGameSystem
{
private:
#pragma region Constants
    static constexpr ResourcePath c_ngPlusQuest = R"(mod\quest\newgameplus.quest)";
    static constexpr ResourcePath c_ngPlusPrologueQuest = R"(mod\quest\newgameplus_q001.quest)";
    static constexpr ResourcePath c_ngPlusStandaloneQuest = R"(mod\quest\newgameplus_standalone.quest)";
    static constexpr ResourcePath c_encounterTestQuest =
        R"(mod\quest\newgameplus\bossencountertest\bossencountertest.quest)";

    static constexpr ResourcePath c_ep1Quest = R"(ep1\quest\ep1.quest)";
    static constexpr ResourcePath c_ep1PreorderQuest = R"(ep1\quest\ep1_preorder.quest)";

    static constexpr ResourcePath c_ngPlusQ001Path = R"(mod\quest\newgameplus_q001.gamedef)";
    static constexpr ResourcePath c_ngPlusQ101Path = R"(mod\quest\newgameplus.gamedef)";
    static constexpr ResourcePath c_ngPlusQ101StandalonePath = R"(mod\quest\newgameplus_standalone.gamedef)";
    static constexpr ResourcePath c_invalidPath = {};

    // Lookup table for game definition resource paths
    static constexpr std::array<ResourcePath, static_cast<std::size_t>(ENGPlusType::Invalid) + 1u>
        c_ngPlusGameDefinitions = {c_ngPlusQ001Path, c_ngPlusQ101Path, c_ngPlusQ101StandalonePath, c_invalidPath,
                                   c_invalidPath};

    static constexpr TweakDBID c_initialLoadingScreen = "InitLoadingScreen.DefaultInitialLoadingScreen";
#pragma endregion

public:
    Handle<NGPlusProgressionData> GetProgressionData()
    {
        return m_progressionData;
    }

    // Only checks for NG+ and NG+ Q001 quests, not standalone
    // Encounter chicken now as well
    bool IsGoodForRandomEncounters()
    {
        return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist || m_isInEncounterTest;
    }

    bool IsInNewGamePlusSave()
    {
        return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist || m_isInNewGamePlusStandalone ||
               m_isInEncounterTest;
    }

    bool IsInNewGamePlusPrologue()
    {
        return m_isInNewGamePlusPrologue;
    }

    bool IsInNewGamePlusHeistOrStandalone()
    {
        return m_isInNewGamePlusHeist || m_isInNewGamePlusStandalone;
    }

    void UpdateNewGamePlusState()
    {
        if (!m_questsSystem)
        {
            return;
        }

        auto& questsList = shared::raw::QuestsSystem::QuestsList::Ref(m_questsSystem);

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
            else if (questResource == c_encounterTestQuest)
            {
                m_isInEncounterTest = true;
                return;
            }
        }
    }

    void LoadExpansionIntoSave()
    {
        if (!m_questsSystem)
        {
            PluginContext::Error("[LoadExpansionIntoSave] m_questsSystem == NULL");
            return;
        }

        shared::raw::QuestsSystem::AddQuest(m_questsSystem, c_ep1Quest);
    }

    void SetNewGamePlusQuest(ENGPlusType aType)
    {
        m_selectedNgPlusType = aType;
    }

    ENGPlusType GetNewGamePlusQuest()
    {
        return m_selectedNgPlusType;
    }

    bool HasPointOfNoReturnSave()
    {
        return files::HasValidPointOfNoReturnSave();
    }

    void RequestHasValidNewGamePlusSaves(WeakHandle<IScriptable> aTarget, CName aCallback)
    {
        files::HasNewGamePlusSaveAsync(aTarget, aCallback);
    }

    bool IsSaveValidForNewGamePlus(Red::ScriptRef<CString>& aSaveName)
    {
        return files::IsValidForNewGamePlus(aSaveName->c_str());
    }

    // TODO: async this, learn how journal gets loaded
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

    void AsyncResolveNewGamePlusSaves(DynArray<CString> aSaves, WeakHandle<IScriptable> aRef, CName aCallbackName)
    {
        struct SaveResult
        {
            bool m_valid{};
            std::uint64_t m_playthroughHash{};
        };

        JobQueue{}.Dispatch(
            [aSaves, aRef, aCallbackName](const JobGroup& aGroup)
            {
                // Lockless, efficient
                // Very nice
                // Is it more efficient than using sync method, though?
                DynArray<SaveResult> results{};
                results.Reserve(aSaves.size);
                results.size = aSaves.size;

                JobQueue delayQueue(aGroup);

                for (auto i = 0u; i < aSaves.size; i++)
                {
                    JobQueue worker(aGroup);
                    worker.Dispatch(
                        [i, &aSaves, &results]()
                        {
                            auto& saveName = aSaves[i];

                            auto metadataPath =
                                files::GetRedPathToSaveFile(saveName.c_str(), files::c_metadataFileName);

                            results[i].m_valid =
                                files::IsValidForNewGamePlus(metadataPath, results[i].m_playthroughHash);
                        });

                    delayQueue.Wait(worker.Capture());
                }

                // Wait for the jobs to complete
                WaitForQueue(delayQueue, std::chrono::milliseconds(1000));

                tsl::hopscotch_set<std::uint64_t> encounteredHashes{};
                DynArray<int> indices{};
                for (auto i = 0u; i < results.size; i++)
                {
                    auto& result = results[i];

                    if (result.m_valid)
                    {
                        if (!encounteredHashes.contains(result.m_playthroughHash))
                        {
                            indices.PushBack(i);
                            encounteredHashes.emplace(result.m_playthroughHash);
                        }
                    }
                }

                if (!aRef.Expired())
                {
                    auto locked = aRef.Lock();

                    CallVirtual(locked, aCallbackName, indices);
                }
            });
    }
#pragma region Transfer
    bool ParsePointOfNoReturnSaveData(Red::ScriptRef<CString>& aSaveName)
    {
        // This needs to be async
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

        PluginContext::Spew("Time taken: {}", duration);

        return true;
    }
#pragma endregion
#pragma region ScriptLogging
    // NOTE: maybe create new "final" mode that has spew logs disabled?
    void Spew(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Spew(aStr->c_str());
    }

    void Error(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Error(aStr->c_str());
    }
#pragma endregion

#pragma region InteriorDetection
    void TickInteriors()
    {
        // Rebuilt IsEntityInInteriorArea

        util::ScopeGuard guard(m_isExterior, false);
        if (!m_playerSystem || !m_questsSystem)
        {
            return;
        }

        if (!m_modConfig.m_enableRandomEncounters || !m_modConfig.m_useExteriorDetectionForRandomEncounters ||
            !m_isGoodForRandomEncounters)
        {
            return;
        }

        Handle<game::Object> player{};
        shared::raw::PlayerSystem::GetPlayerControlledGameObject(m_playerSystem, player);

        if (!player || player->status != EntityStatus::Attached)
        {
            return;
        }

        auto runtimeScene = player->runtimeScene;

        // *(_QWORD *)(*(_QWORD *)(*(_QWORD *)(entityPtr + 0xB8) + 8LL) + 112LL) )
        auto unk1 = shared::util::OffsetPtr<0x8, void*>::Ref(runtimeScene);
        auto unk2 = shared::util::OffsetPtr<0x70, void*>::Ref(unk1);

        if (!unk2)
        {
            return;
        }

        auto& coordinates = player->transformComponent->worldTransform.Position;

        Vector4 positionAsVec4{coordinates.x.Bits * 0.0000076293945f, coordinates.y.Bits * 0.0000076293945f,
                               coordinates.z.Bits * 0.0000076293945f, 0};

        int result{};

        guard.SetEnabled(false);

        m_isExterior = *shared::raw::World::IsInInterior(unk2, &result, coordinates, false) == 0 &&
                       !shared::raw::SafeAreaManager::IsPointInSafeArea(m_safeAreaManager, positionAsVec4);
    }

    void TickFactsDB()
    {
        if (m_questsSystem)
        {
            shared::raw::QuestsSystem::FactsDB(m_questsSystem)
                ->SetFact("ngplus_is_outside", static_cast<int>(m_isExterior));
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

        static auto s_nodeName = shared::util::NamePoolRegistrar<"NewGamePlusSystem">::Get();

        shared::raw::Save::NodeAccessor node(stream, s_nodeName, false, false);

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

            if (version >= ENewGamePlusSaveVersion::RandomEncounterChickenTestInDetection)
            {
                stream->ReadWriteEx(&m_isInEncounterTest);
            }
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

        static auto s_nodeName = shared::util::NamePoolRegistrar<"NewGamePlusSystem">::Get();

        shared::raw::Save::NodeAccessor node(stream, s_nodeName, true, false);

        auto version = ENewGamePlusSaveVersion::RandomEncounterChickenTestInDetection;

        stream->ReadWriteEx(&version);
        stream->ReadWriteEx(&m_isInNewGamePlusPrologue);
        stream->ReadWriteEx(&m_isInNewGamePlusHeist);
        stream->ReadWriteEx(&m_isInNewGamePlusStandalone);
        stream->ReadWriteEx(&m_isInEncounterTest);
    }

    void OnGamePrepared() override
    {
        m_playerSystem = GetGameSystem<cp::PlayerSystem>();
        m_questsSystem = GetGameSystem<quest::QuestsSystem>();
        m_safeAreaManager = GetGameSystem<AI::SafeAreaManager>();

        m_modConfig = settings::GetRandomEncounterSettings();
        // If we haven't restored, update NG+ config
        if (!m_restoredDataFromSave)
        {
            UpdateNewGamePlusState();
        }

        m_isGoodForRandomEncounters = IsGoodForRandomEncounters();
    }

    void OnWorldDetached(world::RuntimeScene* aScene) override
    {
        // Clean up state

        m_playerSystem = nullptr;
        m_questsSystem = nullptr;
        m_safeAreaManager = nullptr;

        m_isGoodForRandomEncounters = false;

        m_isInNewGamePlusPrologue = false;
        m_isInNewGamePlusHeist = false;
        m_isInNewGamePlusStandalone = false;
        m_isInEncounterTest = false;

        m_isExterior = false;
        m_restoredDataFromSave = false;
    }
#pragma endregion

#pragma region SessionLauncher
    void LaunchNewGamePlus(Handle<game::ui::CharacterCustomizationState> aState)
    {
        auto selectedGameDefinition = c_ngPlusGameDefinitions[static_cast<std::uint32_t>(m_selectedNgPlusType)];

        if (selectedGameDefinition == c_invalidPath)
        {
            return;
        }

        // What the game does (a bit) before game definition load request
        shared::raw::Ink::InkSystem::Get()->SetInitialLoadingScreenTDBID(c_initialLoadingScreen);

        // Note: IDK if we should also update NG+ type here
        ResourceLoader::Get()
            ->LoadAsync(selectedGameDefinition)
            ->OnLoaded(
                [this, aState](Handle<CResource>& aResource)
                {
                    auto& asGameDef = Cast<gsm::GameDefinition>(aResource);
                    if (!asGameDef)
                    {
                        return;
                    }

                    game::GameSessionDesc desc{};

                    shared::raw::GameDefinition::ToWorldID(asGameDef, &desc.worldId);

                    const auto hasExpansion = CGameEngine::Get()->isEP1;

                    for (auto& mainQuest : asGameDef->mainQuests)
                    {
                        if (!hasExpansion && (mainQuest->questFile.path == c_ep1Quest ||
                                              mainQuest->questFile.path == c_ep1PreorderQuest))
                        {
                            // Skip over EP1/EP1 preorder if user does not own expansion
                            continue;
                        }

                        game::MainQuestData data{};

                        data.questPath = mainQuest->questFile.path;
                        data.questPathValid = data.questPath != c_invalidPath;

                        if (mainQuest->additionalContent)
                        {
                            data.additionalContentId = mainQuest->additionalContentName;
                        }

                        desc.mainQuests.PushBack(data);
                    }

                    desc.characterCustomizationState = MakeScriptedHandle<game::ui::CharacterCustomizationState>(
                        GetClass<game::ui::CharacterCustomizationState>());

                    // Avoid direct handle copy, probably would be a bad idea
                    desc.characterCustomizationState->GetType()->Assign(desc.characterCustomizationState.instance,
                                                                        aState.instance);

                    shared::raw::Ink::SessionData::Data sessionData{};

                    sessionData.AddArgument("gameSessionDesc", &desc);
                    sessionData.AddArgument("spawnTags", &asGameDef->spawnPointTags);

                    auto systemRequestsHandler = shared::raw::Ink::InkSystem::Get()->m_requestsHandler.Lock();

                    auto inputDeviceId =
                        shared::raw::Ink::SystemRequestsHandler::InputDeviceId::Ref(systemRequestsHandler);

                    // Why does this get set in all session transfers? IDK
                    sessionData.AddArgument("inputDeviceID", &inputDeviceId);

                    shared::raw::Ink::SystemRequestsHandler::StartSession(systemRequestsHandler, &sessionData);
                });
    }
#pragma endregion
private:
#pragma region Progression
    Handle<NGPlusProgressionData> m_progressionData{};
#pragma endregion

#pragma region IngameContext
    settings::ModConfig m_modConfig{};

    bool m_restoredDataFromSave{};

    bool m_isInNewGamePlusPrologue{};
    bool m_isInNewGamePlusHeist{};
    bool m_isInNewGamePlusStandalone{};
    bool m_isInEncounterTest{};

    bool m_isExterior{};
    bool m_isGoodForRandomEncounters{};
#pragma endregion

#pragma region Systems
    cp::PlayerSystem* m_playerSystem{};
    quest::QuestsSystem* m_questsSystem{};
    AI::SafeAreaManager* m_safeAreaManager{};
#pragma endregion

#pragma region Session
    ENGPlusType m_selectedNgPlusType = ENGPlusType::Invalid;
#pragma endregion

    RTTI_IMPL_TYPEINFO(NewGamePlusSystem);
    RTTI_IMPL_ALLOCATOR();
};

} // namespace mod

RTTI_DEFINE_ENUM(mod::ENewGamePlusStartType);
RTTI_DEFINE_ENUM(mod::ENGPlusType);

RTTI_DEFINE_CLASS(mod::NewGamePlusSystem, {
    RTTI_METHOD(ParsePointOfNoReturnSaveData);
    RTTI_METHOD(HasPointOfNoReturnSave);

    RTTI_METHOD(RequestHasValidNewGamePlusSaves);
    RTTI_METHOD(AsyncResolveNewGamePlusSaves);
    RTTI_METHOD(ResolveNewGamePlusSaves);

    RTTI_METHOD(GetProgressionData);
    RTTI_METHOD(IsSaveValidForNewGamePlus);

    RTTI_METHOD(LoadExpansionIntoSave);

    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);

    RTTI_METHOD(IsInNewGamePlusSave);
    RTTI_METHOD(IsInNewGamePlusPrologue);
    RTTI_METHOD(IsInNewGamePlusHeistOrStandalone);

    RTTI_METHOD(LaunchNewGamePlus);
    RTTI_METHOD(SetNewGamePlusQuest);
    RTTI_METHOD(GetNewGamePlusQuest);
});