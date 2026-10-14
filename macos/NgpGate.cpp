#include "NgpFeatures.hpp"

#include <cstdio>
#include <mutex>
#include <set>
#include <string>

#include <Shared/Raw/FileSystem/FileSystem.hpp>
#include <Shared/Raw/GameDefinition/GameDefinition.hpp>
#include <Shared/Raw/Ink/InkSystem.hpp>
#include <Shared/Raw/PlayerSystem/PlayerSystem.hpp>
#include <Shared/Raw/Quest/NodeFuncs.hpp>
#include <Shared/Raw/Quest/QuestsSystem.hpp>
#include <Shared/Raw/Save/Save.hpp>
#include <Shared/Raw/World/SafeAreaManager.hpp>
#include <Shared/Raw/World/World.hpp>

namespace
{
template <typename... Ts>
bool AllAvailable(const char* aFeature, const Ts&... aFns)
{
    uint32_t missing[sizeof...(Ts) == 0 ? 1 : sizeof...(Ts)]{};
    int count = 0;
    auto consider = [&](const auto& fn) {
        using Fn = std::decay_t<decltype(fn)>;
        if (!Fn::IsAvailable())
        {
            missing[count++] = static_cast<uint32_t>(Fn::target);
        }
    };
    (consider(aFns), ...);
    if (count == 0)
    {
        return true;
    }
    std::fprintf(stderr, "[ngplus] feature %s SKIPPED, missing hashes: %u", aFeature, missing[0]);
    for (int i = 1; i < count; ++i)
    {
        std::fprintf(stderr, ", %u", missing[i]);
    }
    std::fprintf(stderr, "\n");
    return false;
}
}

namespace ngp::mac
{
bool SaveReadingAvailable()
{
    // One gate for the whole save-reading path. Any miss means "has an NG+ save" is false.
    // The buffered stream dtor (2577866805) is inlined. It is not in this list and it is not called.
    // The metadata file is read by LoadSaveMetadata's Apple branch (SaveFS.cpp), not by the game's loader, which has
    // no macOS entry. The NodeAccessor pair is only used by the disabled TestSaveStream block and by the NG+ save
    // node, which has its own inline Apple implementation.
    static const bool ok = AllAvailable(
        "save reading", shared::raw::Filesystem::Path::GetSaveFolder, shared::raw::Filesystem::Path::MergeDirToPath,
        shared::raw::Filesystem::Path::MergeFileToPath, shared::raw::Filesystem::RedFileManager::Instance,
        shared::raw::Filesystem::RedFileManager::FindFiles, shared::raw::Filesystem::RedFileManager::OpenFileStreamInternal);
    return ok;
}

bool InteriorsAvailable()
{
    static const bool ok = AllAvailable("TickInteriors", shared::raw::PlayerSystem::GetPlayerControlledGameObject,
                                        shared::raw::World::IsInInterior, shared::raw::SafeAreaManager::IsPointInSafeArea);
    return ok;
}

bool LaunchAvailable()
{
    static const bool ok =
        AllAvailable("LaunchNewGamePlus", shared::raw::GameDefinition::ToWorldID, shared::raw::Ink::InkSystemInstance,
                     shared::raw::Ink::SystemRequestsHandler::StartSession,
                     shared::raw::Ink::SessionData::Data::AddArgumentInternal);
    // SessionData's destructor is inlined on macOS (SharedPunk's InkSystem.cpp releases the arguments itself), and
    // the loading-screen picture is optional (SetInitialLoadingScreenTDBID skips it while its two functions are missing).
    return ok;
}

bool NodeAccessorAvailable()
{
    // The game inlines NodeAccessor on macOS; SharedPunk's Save.cpp has the Apple copy, so nothing has to resolve.
    return true;
}

bool AddQuestAvailable()
{
    static const bool ok = AllAvailable("LoadExpansionIntoSave", shared::raw::QuestsSystem::AddQuest);
    return ok;
}

void LogUnconfirmed(const char* aSite)
{
    static std::mutex s_lock;
    static std::set<std::string> s_seen;
    std::lock_guard guard(s_lock);
    if (!s_seen.insert(aSite).second)
    {
        return;
    }
    std::fprintf(stderr, "[ngplus] feature %s SKIPPED: SetFact slot and QuestMutex are unconfirmed\n", aSite);
}

void LogDedupeSkipped()
{
    static bool s_once = false;
    if (s_once)
    {
        return;
    }
    s_once = true;
    std::fprintf(stderr, "[ngplus] feature quest dedupe SKIPPED, LoadQuestResources hash 407969320 is inlined\n");
}
}
