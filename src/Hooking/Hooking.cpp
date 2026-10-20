#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <Context/Context.hpp>

#include <Shared/Hooks/HookManager.hpp>
#include <Shared/Raw/Telemetry/Telemetry.hpp>
#include <Shared/Raw/Quest/QuestsSystem.hpp>
#include <Shared/Raw/CharacterCustomizationSystem/CharacterCustomizationSystem.hpp>
#include <Shared/Raw/CharacterCustomizationState/CharacterCustomizationState.hpp>

#include <tsl/hopscotch_set.h>
#ifdef __APPLE__
#include "MacShims.hpp"
#include "NgpFeatures.hpp"
#endif

using namespace Red;

namespace hooking
{
namespace QuestsSystem
{
// This should be in Sharedpunk but lazy
constexpr auto HashLoadQuestResources = 407969320u;
constexpr auto LoadQuestResources =
    shared::util::RawFunc<HashLoadQuestResources, void (*)(quest::QuestsSystem*, JobQueue&, bool&)>();

void OnLoadQuestResources(quest::QuestsSystem* aQuestsSystem, JobQueue& aQueue, bool& aSuccess) {
    // Dedupe quests list - maybe this works for bugfix of 9999 root quests? Not sure
    // No need for mutex here I think
    tsl::hopscotch_set<ResourcePath> quests{};
    DynArray<ResourcePath> dedupedQuestList{};

    auto& questsList = shared::raw::QuestsSystem::QuestsList::Ref(aQuestsSystem);

    for (auto i : questsList)
    {
        if (!quests.contains(i))
        {
            dedupedQuestList.PushBack(i);
            quests.insert(i);
        }
    }

    questsList = dedupedQuestList;
}
}

namespace LoadFacts
{
void OnLoadTelemetryFactMap(HashMap<std::uint32_t, CString>& aMap)
{
    // We add our own facts to the gatherer
#ifdef __APPLE__
    // The Mac function fills the global at image offset 0x8132140. The argument is not that map.
    (void)aMap;
    auto* map = reinterpret_cast<HashMap<std::uint32_t, CString>*>(sharedpunk_mac_image_base() + 0x8132140);
    map->Insert(FNV1a32("ngplus_active"), "ngplus_active");
    map->Insert(FNV1a32("ngplus_q001_start"), "ngplus_q001_start");
    map->Insert(FNV1a32("ngplus_standalone_q101_start"), "ngplus_standalone_q101_start");
#else
    aMap.Insert(FNV1a32("ngplus_active"), "ngplus_active");
    aMap.Insert(FNV1a32("ngplus_q001_start"), "ngplus_q001_start");
    aMap.Insert(FNV1a32("ngplus_standalone_q101_start"), "ngplus_standalone_q101_start");
#endif
}
} // namespace LoadFacts

namespace CharacterCustomizationSystem
{
void OnNewGame(game::ui::CharacterCustomizationSystem* aThis, void* aRdx)
{
    // FIX: Fresh Start integration breaking things
    // Implement a more robust fix using FDB mechanics used for setting cp77_new_game
    auto& spinlock = shared::raw::CharacterCustomizationSystem::Lock::Ref(aThis);

    // Lock state
    std::unique_lock lock(spinlock);

    auto& state = shared::raw::CharacterCustomizationSystem::State::Ref(aThis);

    if (!state)
    {
        PluginContext::Error("hooking::CharacterCustomizationSystem::OnNewGame, CC state == NULL");
        return;
    }

    // No point running anything if we're not starting new game
    // This shouldn't persist in save, so it shouldn't be running lots of times
    if (!shared::raw::CharacterCustomizationState::IsNewGame(state))
    {
        return;
    }
    
    TweakDBID lifepathId{};

    shared::raw::CharacterCustomizationState::GetLifePath(state, lifepathId);

#ifdef __APPLE__
    if constexpr (!ngp::mac::kEnableUnconfirmedFactAndMutex)
    {
        ngp::mac::LogUnconfirmed("CharacterCustomization OnNewGame SetFact");
        return;
    }
#endif
    auto questsSystem = GetGameSystem<quest::QuestsSystem>();
    auto factsDB = shared::raw::QuestsSystem::FactsDB(questsSystem);

    if (lifepathId == "LifePaths.Corporate")
    {
        factsDB->SetFact("ngplus_is_corpo", 1);
    }
    else if (lifepathId == "LifePaths.StreetKid")
    {
        factsDB->SetFact("ngplus_is_streetkid", 1);
    }
    else if (lifepathId == "LifePaths.Nomad")
    {
        factsDB->SetFact("ngplus_is_nomad", 1);
    }
    else if (lifepathId == "LifePaths.NewStart")
    {
        factsDB->SetFact("ngplus_fresh_start_on", 1);
    }

    factsDB->SetFact("ngplus_cc_on_new_game_finished", 1);
}
}

bool InitializeHooking()
{
    // Note: OrDie here really doesn't do anything!
#ifdef __APPLE__
    ngp::mac::LogDedupeSkipped();
#endif
    shared::hook::HookAfter<shared::raw::Telemetry::LoadFactMap>(&LoadFacts::OnLoadTelemetryFactMap)
        .OrDie("Failed to hook Telemetry::LoadFacts");

    shared::hook::HookAfter<shared::raw::CharacterCustomizationSystem::OnNewGame>(
        &CharacterCustomizationSystem::OnNewGame)
        .OrDie("Failed to hook CharacterCustomizationSystem::OnNewGame");

#ifndef __APPLE__
    shared::hook::HookBefore<QuestsSystem::LoadQuestResources>(&QuestsSystem::OnLoadQuestResources)
        .OrDie("Failed to hook QuestsSystem::LoadQuestResources");
#endif

    return true;
}

bool DetachHooking()
{
    return shared::hook::Unhook<shared::raw::Telemetry::LoadFactMap>() && shared::hook::Unhook<shared::raw::CharacterCustomizationSystem::OnNewGame>();
}
} // namespace hooking