#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/AI/SafeAreaManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/cp/PlayerSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationState.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <GameSystem/Definitions/PlayerSaveData.hpp>
#include <Util/Settings/SettingsAccessor.hpp>

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

/**
 * \brief A wrapper for some of the mod's functionality.
 */
class NewGamePlusSystem : public Red::IGameSystem
{
private:
#pragma region Constants
    static constexpr Red::ResourcePath NgPlusQuest = R"(mod\quest\newgameplus.quest)";
    static constexpr Red::ResourcePath NgPlusPrologueQuest = R"(mod\quest\newgameplus_q001.quest)";
    static constexpr Red::ResourcePath NgPlusStandaloneQuest = R"(mod\quest\newgameplus_standalone.quest)";
    static constexpr Red::ResourcePath EncounterTestQuest =
        R"(mod\quest\newgameplus\bossencountertest\bossencountertest.quest)";

    static constexpr Red::ResourcePath Ep1Quest = R"(ep1\quest\ep1.quest)";
    static constexpr Red::ResourcePath Ep1PreorderQuest = R"(ep1\quest\ep1_preorder.quest)";

    static constexpr Red::ResourcePath NgPlusQ001Path = R"(mod\quest\newgameplus_q001.gamedef)";
    static constexpr Red::ResourcePath NgPlusQ101Path = R"(mod\quest\newgameplus.gamedef)";
    static constexpr Red::ResourcePath NgPlusQ101StandalonePath = R"(mod\quest\newgameplus_standalone.gamedef)";
    static constexpr Red::ResourcePath InvalidPath = {};

    // Lookup table for game definition resource paths
    static constexpr std::array<Red::ResourcePath, static_cast<std::size_t>(ENGPlusType::Invalid) + 1u>
        NgPlusGameDefinitions = {NgPlusQ001Path, NgPlusQ101Path, NgPlusQ101StandalonePath, InvalidPath, InvalidPath};

    static constexpr Red::TweakDBID InitialLoadingScreen = "InitLoadingScreen.DefaultInitialLoadingScreen";

    static constexpr Red::TweakDBID FreshStartLifePathID = "LifePaths.NewStart";
#pragma endregion

#pragma region Statics
    // Note: Game systems are kept indefinitely, we can cache thisptr from OnInitialize here
    static inline NewGamePlusSystem* s_this;
#pragma endregion
public:
#pragma region API
    static NewGamePlusSystem* GetInstance() noexcept;

    settings::ModConfig GetConfig() noexcept;
#pragma endregion

#pragma region Redscript
    void ToggleDebugMode();

    bool IsInNewGamePlusSave();

    bool IsInNewGamePlusPrologue();

    bool IsInNewGamePlusHeistOrStandalone();

    void UpdateNewGamePlusState();

    void LoadExpansionIntoSave();

    void SetNewGamePlusQuest(ENGPlusType aType);

    ENGPlusType GetNewGamePlusQuest();

    bool HasPointOfNoReturnSave();

    void RequestHasValidNewGamePlusSaves(Red::WeakHandle<Red::IScriptable> aTarget, Red::CName aCallback);

    Red::Handle<NGPlusProgressionData> GetProgressionData();
    Red::DynArray<int> ResolveNewGamePlusSaves(Red::ScriptRef<Red::DynArray<Red::CString>>& aSaves);
    void RequestResolveNewGamePlusSaves(Red::DynArray<Red::CString> aSaves, Red::WeakHandle<IScriptable> aRef,
                                        Red::CName aCallbackName);

    void RequestLoadSaveData(Red::CString aSaveName, Red::WeakHandle<IScriptable> aTarget, Red::CName aCallbackName);

    void Spew(Red::ScriptRef<Red::CString>& aStr);
    void Error(Red::ScriptRef<Red::CString>& aStr);

    void LaunchNewGamePlus(Red::Handle<Red::game::ui::CharacterCustomizationState> aState);
#pragma endregion

#pragma region OnTick
    void TickInteriors();
    void TickFactsDB();

    void OnTick(Red::FrameInfo& aFrame, Red::JobQueue& aJob);
#pragma endregion

#pragma region Overrides
    void OnRegisterUpdates(Red::UpdateRegistrar* aRegistrar) override;

    void OnGameLoad(const Red::JobGroup& aJobGroup, bool& aSuccess, void* aStream) override;
    void OnGameSave(void* aStream) override;

    std::uint32_t OnBeforeGameSave(const Red::JobGroup& aJobGroup, void* aMetadataObject) override;

    void OnGamePrepared() override;
    void OnWorldDetached(Red::world::RuntimeScene* aScene) override;
    void OnInitialize(const Red::JobHandle& aJob) override;
#pragma endregion

#pragma region Internal
private:
    bool LoadSaveData(const Red::CString& aSaveName);
    bool IsGoodForRandomEncounters();
#pragma endregion
private:
#pragma region Progression
    Red::Handle<NGPlusProgressionData> m_progressionData{};
#pragma endregion

#pragma region Ingame context
    Red::SharedSpinLock m_configLock{};
    settings::ModConfig m_modConfig{};

    bool m_restoredDataFromSave{};

    bool m_isInNewGamePlusPrologue{};
    bool m_isInNewGamePlusHeist{};
    bool m_isInNewGamePlusStandalone{};
    bool m_isInEncounterTest{};

    bool m_isExterior{};
    bool m_isGoodForRandomEncounters{};
#pragma endregion

#pragma region Game systems
    Red::cp::PlayerSystem* m_playerSystem{};
    Red::quest::QuestsSystem* m_questsSystem{};
    Red::AI::SafeAreaManager* m_safeAreaManager{};
#pragma endregion

#pragma region Session
    ENGPlusType m_selectedNgPlusType = ENGPlusType::Invalid;
#pragma endregion

    RTTI_IMPL_TYPEINFO(NewGamePlusSystem);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace mod
