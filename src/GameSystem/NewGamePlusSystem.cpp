#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <GameSystem/NewGamePlusSystem.hpp>

#include <RED4ext/Scripting/Natives/Generated/gsm/MainQuest.hpp>
#include <RED4ext/Scripting/Natives/gameGameSessionDesc.hpp>

#include <RED4ext/Scripting/Natives/entIPlacedComponent.hpp>

#include <context/context.hpp>

#include <filesystem/SaveFS.hpp>

#include <util/scopeguard.hpp>
#include <util/settings/settingsAccessor.hpp>

#include <Shared/Raw/CharacterCustomizationState/CharacterCustomizationState.hpp>
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

#include <chrono>
#include <unordered_set>

using namespace Red;

namespace Internal
{
mod::NewGamePlusSystem* s_instance{};
}

mod::NewGamePlusSystem* mod::NewGamePlusSystem::GetInstance() noexcept
{
    // Note: maybe assert if s_this == nullptr?
    return Internal::s_instance;
}

settings::ModConfig mod::NewGamePlusSystem::GetConfig() noexcept
{
    std::shared_lock lock(m_configLock);

    return m_modConfig;
}

void mod::NewGamePlusSystem::ToggleDebugMode()
{
    PluginContext::m_isDebugModeEnabled = !PluginContext::m_isDebugModeEnabled;
}

Handle<NGPlusProgressionData> mod::NewGamePlusSystem::GetProgressionData()
{
    return m_progressionData;
}

// Only checks for NG+ and NG+ Q001 quests, not standalone
// Encounter chicken now as well
bool mod::NewGamePlusSystem::IsGoodForRandomEncounters()
{
    return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist || m_isInEncounterTest;
}

bool mod::NewGamePlusSystem::IsInNewGamePlusSave()
{
    return m_isInNewGamePlusPrologue || m_isInNewGamePlusHeist || m_isInNewGamePlusStandalone || m_isInEncounterTest;
}

bool mod::NewGamePlusSystem::IsInNewGamePlusPrologue()
{
    return m_isInNewGamePlusPrologue;
}

bool mod::NewGamePlusSystem::IsInNewGamePlusHeistOrStandalone()
{
    return m_isInNewGamePlusHeist || m_isInNewGamePlusStandalone;
}

void mod::NewGamePlusSystem::UpdateNewGamePlusState()
{
    if (!m_questsSystem)
    {
        return;
    }

    auto& questsList = shared::raw::QuestsSystem::QuestsList::Ref(m_questsSystem);

    // We can't have multiple NG+ quests in the same game, so returning immediately is fine
    for (auto questResource : questsList)
    {
        if (questResource == NgPlusQuest)
        {
            m_isInNewGamePlusHeist = true;
            return;
        }
        else if (questResource == NgPlusPrologueQuest)
        {
            m_isInNewGamePlusPrologue = true;
            return;
        }
        else if (questResource == NgPlusStandaloneQuest)
        {
            m_isInNewGamePlusStandalone = true;
            return;
        }
        else if (questResource == EncounterTestQuest)
        {
            m_isInEncounterTest = true;
            return;
        }
    }
}

void mod::NewGamePlusSystem::LoadExpansionIntoSave()
{
    if (!m_questsSystem)
    {
        PluginContext::Error("[LoadExpansionIntoSave] m_questsSystem == NULL");
        return;
    }

    shared::raw::QuestsSystem::AddQuest(m_questsSystem, Ep1Quest);
}

void mod::NewGamePlusSystem::SetNewGamePlusQuest(ENGPlusType aType)
{
    m_selectedNgPlusType = aType;
}

mod::ENGPlusType mod::NewGamePlusSystem::GetNewGamePlusQuest()
{
    return m_selectedNgPlusType;
}

bool mod::NewGamePlusSystem::HasPointOfNoReturnSave()
{
    return false;
}

void mod::NewGamePlusSystem::RequestHasValidNewGamePlusSaves(WeakHandle<IScriptable> aTarget, CName aCallback)
{
    files::HasNewGamePlusSaveAsync(aTarget, aCallback);
}

DynArray<int> mod::NewGamePlusSystem::ResolveNewGamePlusSaves(Red::ScriptRef<DynArray<CString>>& aSaves)
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

// TODO: learn how journal gets loaded, use that + FactsDB for save file validation
void mod::NewGamePlusSystem::RequestResolveNewGamePlusSaves(DynArray<CString> aSaves, WeakHandle<IScriptable> aRef,
                                                            CName aCallbackName)
{
    struct SaveResult
    {
        bool m_valid{};
        std::uint64_t m_playthroughHash{};
    };

    Red::JobQueue{}.Dispatch(
        [aSaves, aRef, aCallbackName](const JobGroup& aGroup)
        {
            // Lockless, efficient
            // Very nice
            // Is it more efficient than using sync method, though?
            DynArray<SaveResult> results{};
            results.Reserve(aSaves.size);
            results.size = aSaves.size;

            Red::JobQueue delayQueue(aGroup);

            for (auto i = 0u; i < aSaves.size; i++)
            {
                Red::JobQueue worker(aGroup);
                worker.Dispatch(
                    [i, &aSaves, &results]()
                    {
                        auto& saveName = aSaves[i];

                        auto metadataPath = files::GetRedPathToSaveFile(saveName.c_str(), files::c_metadataFileName);

                        results[i].m_valid = files::IsValidForNewGamePlus(metadataPath, results[i].m_playthroughHash);
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
bool mod::NewGamePlusSystem::LoadSaveData(const CString& aSaveName)
{
    auto start = std::chrono::high_resolution_clock{}.now();

    parser::Parser parser{};

    if (!parser.ParseSavegame(aSaveName))
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

void mod::NewGamePlusSystem::RequestLoadSaveData(CString aSaveName, WeakHandle<IScriptable> aTarget,
                                                 CName aCallbackName)
{
    Red::JobQueue{}.Dispatch(
        [this, aSaveName, aTarget, aCallbackName]()
        {
            const auto returnValue = LoadSaveData(aSaveName);

            if (!aTarget.Expired())
            {
                auto ref = aTarget.Lock();

                CallVirtual(ref, aCallbackName, returnValue);
            }
        });
}
#pragma endregion
#pragma region ScriptLogging
// NOTE: maybe create new "final" mode that has spew logs disabled?
void mod::NewGamePlusSystem::Spew(Red::ScriptRef<CString>& aStr)
{
    PluginContext::Spew(aStr->c_str());
}

void mod::NewGamePlusSystem::Error(Red::ScriptRef<CString>& aStr)
{
    PluginContext::Error(aStr->c_str());
}
#pragma endregion

#pragma region InteriorDetection
void mod::NewGamePlusSystem::TickInteriors()
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

void mod::NewGamePlusSystem::TickFactsDB()
{
    if (m_questsSystem)
    {
        shared::raw::QuestsSystem::FactsDB(m_questsSystem)
            ->SetFact("ngplus_is_outside", static_cast<int>(m_isExterior));
    }
}
#pragma endregion

#pragma region Overrides
void mod::NewGamePlusSystem::OnTick(FrameInfo& aFrame, Red::JobQueue& aJob)
{
    TickInteriors();
    TickFactsDB();
}

void mod::NewGamePlusSystem::OnRegisterUpdates(UpdateRegistrar* aRegistrar)
{
    aRegistrar->RegisterUpdate(UpdateTickGroup::EntityUpdateState, this, "NewGamePlusSystem/Tick",
                               [this](FrameInfo& aFrame, Red::JobQueue& aJob) { this->OnTick(aFrame, aJob); });
}

void mod::NewGamePlusSystem::OnGameLoad(const JobGroup& aJobGroup, bool& aSuccess, void* aStream)
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

std::uint32_t mod::NewGamePlusSystem::OnBeforeGameSave(const JobGroup& aJobGroup, void* aMetadataObject)
{
    // Unfortunately we can't add more metadata from mod, would be very useful
    return static_cast<std::uint32_t>(IsInNewGamePlusSave()); // We only need to do anything when NG+ is active
}

void mod::NewGamePlusSystem::OnGameSave(void* aStream)
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

void mod::NewGamePlusSystem::OnGamePrepared()
{
    m_playerSystem = GetGameSystem<cp::PlayerSystem>();
    m_questsSystem = GetGameSystem<quest::QuestsSystem>();
    m_safeAreaManager = GetGameSystem<AI::SafeAreaManager>();

    {
        std::unique_lock lock(m_configLock);
        m_modConfig = settings::GetModSettings();
    }

    // If we haven't restored, update NG+ config
    if (!m_restoredDataFromSave)
    {
        UpdateNewGamePlusState();
    }

    m_isGoodForRandomEncounters = IsGoodForRandomEncounters();
}

void mod::NewGamePlusSystem::OnWorldDetached(world::RuntimeScene* aScene)
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

void mod::NewGamePlusSystem::OnInitialize(const JobHandle& aJob)
{
    // We only really need this to init thisptr accessor
    // Note that game systems do not get dtord
    Internal::s_instance = this;
}
#pragma endregion

#pragma region SessionLauncher
void mod::NewGamePlusSystem::LaunchNewGamePlus(Handle<game::ui::CharacterCustomizationState> aState)
{
    auto selectedGameDefinition = NgPlusGameDefinitions[static_cast<std::uint32_t>(m_selectedNgPlusType)];

    if (selectedGameDefinition == InvalidPath)
    {
        return;
    }

    // What the game does (a bit) before game definition load request
    shared::raw::Ink::InkSystem::Get()->SetInitialLoadingScreenTDBID(InitialLoadingScreen);

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
                    if (!hasExpansion &&
                        (mainQuest->questFile.path == Ep1Quest || mainQuest->questFile.path == Ep1PreorderQuest))
                    {
                        // Skip over EP1/EP1 preorder if user does not own expansion
                        continue;
                    }

                    game::MainQuestData data{};

                    data.questPath = mainQuest->questFile.path;
                    data.questPathValid = data.questPath != InvalidPath;

                    if (mainQuest->additionalContent)
                    {
                        data.additionalContentId = mainQuest->additionalContentName;
                    }

                    desc.mainQuests.PushBack(data);
                }

                desc.characterCustomizationState = aState;

                shared::raw::Ink::SessionData::Data sessionData{};

                sessionData.AddArgument("gameSessionDesc", &desc);

                TweakDBID lifePath{};
                shared::raw::CharacterCustomizationState::GetLifePath(aState, lifePath);

                // Fresh Start compat for spawn tags (yes, this will start the game at Q000! Enum name becomes a bit
                // misleading...)
                if (m_selectedNgPlusType == ENGPlusType::StartFromQ001 && lifePath == FreshStartLifePathID)
                {
                    const auto spawnTag = *TweakDB::Get()
                                               ->GetFlatValue(TweakDBID(FreshStartLifePathID, ".newGameSpawnTag"))
                                               ->GetValue<CName>();

                    red::TagList spawnTagList{};

                    spawnTagList.Add(spawnTag);

                    sessionData.AddArgument("spawnTags", &spawnTagList);
                }
                else
                {
                    sessionData.AddArgument("spawnTags", &asGameDef->spawnPointTags);
                }

                auto systemRequestsHandler = shared::raw::Ink::InkSystem::Get()->m_requestsHandler.Lock();

                auto inputDeviceId = shared::raw::Ink::SystemRequestsHandler::InputDeviceId::Ref(systemRequestsHandler);

                // Why does this get set in all session transfers? IDK
                sessionData.AddArgument("inputDeviceID", &inputDeviceId);

                shared::raw::Ink::SystemRequestsHandler::StartSession(systemRequestsHandler, &sessionData);
            });
}

RTTI_DEFINE_ENUM(mod::ENewGamePlusStartType);
RTTI_DEFINE_ENUM(mod::ENGPlusType);

RTTI_DEFINE_CLASS(mod::NewGamePlusSystem, {
    RTTI_METHOD(RequestLoadSaveData);
    RTTI_METHOD(RequestHasValidNewGamePlusSaves);
    RTTI_METHOD(RequestResolveNewGamePlusSaves);
    RTTI_METHOD(ResolveNewGamePlusSaves);

    RTTI_METHOD(GetProgressionData);

    RTTI_METHOD(LoadExpansionIntoSave);

    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);

    RTTI_METHOD(IsInNewGamePlusSave);
    RTTI_METHOD(IsInNewGamePlusPrologue);
    RTTI_METHOD(IsInNewGamePlusHeistOrStandalone);

    RTTI_METHOD(LaunchNewGamePlus);
    RTTI_METHOD(SetNewGamePlusQuest);
    RTTI_METHOD(GetNewGamePlusQuest);

    RTTI_METHOD(ToggleDebugMode);
});