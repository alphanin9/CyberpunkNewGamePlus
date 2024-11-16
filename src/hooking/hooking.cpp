#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <context/context.hpp>

#include <Shared/Hooks/HookManager.hpp>
#include <Shared/Raw/GameDefinition/GameDefinition.hpp>
#include <Shared/Raw/Ink/InkSystem.hpp>
#include <Shared/Raw/Telemetry/Telemetry.hpp>

using namespace Red;

namespace hooking
{
namespace SelectGameDefinition
{
enum class GamedefType : char
{
    Basegame = 0,
    EP1 = 1,
    EP1_Standalone = 2,
    GamedefTypesMax = 3
};

ResourcePath* m_detourFn(shared::raw::GameDefinition::SelectMainGameDefinition aCallback, ResourcePath* aDepotPath,
                         char aGamedefType)
{
    auto type = static_cast<GamedefType>(aGamedefType);

    if (!PluginContext::m_isNewGamePlusActive || !PluginContext::m_isInStartNewGame)
    {
        return aCallback(aDepotPath, aGamedefType);
    }

    // Fix for non-EP1 NG+ start
    if (type >= GamedefType::EP1_Standalone)
    {
        return aCallback(aDepotPath, aGamedefType);
    }

    *aDepotPath = PluginContext::m_ngPlusGameDefinitionHash;

    PluginContext::m_isNewGamePlusActive = false;
    return aDepotPath;
}
} // namespace SelectGameDefinition

namespace StartNewGame
{
void __fastcall m_detourFn(shared::raw::Ink::SystemRequestsHandler::StartNewGame aCallback,
                           ink::ISystemRequestsHandler* aThis, Handle<IScriptable>& aState)
{
    PluginContext::m_isInStartNewGame = true;
    aCallback(aThis, aState);
    PluginContext::m_isInStartNewGame = false;
}
} // namespace StartNewGame

namespace LoadFacts
{
void m_detourFn(HashMap<std::uint32_t, CString>& aMap)
{
    // We add our own facts to the gatherer
    aMap.Insert(FNV1a32("ngplus_active"), "ngplus_active");
    aMap.Insert(FNV1a32("ngplus_q001_start"), "ngplus_q001_start");
    aMap.Insert(FNV1a32("ngplus_standalone_q101_start"), "ngplus_standalone_q101_start");
}
} // namespace LoadFacts
bool InitializeHooking()
{
    // TODO: move this to proper game session transition using system from replay
    // As soon as we have initial loading screen implemented
    // Well we have it now

    shared::hook::HookWrap<shared::raw::GameDefinition::SelectMainGameDefinitionFn>(&SelectGameDefinition::m_detourFn)
        .OrDie("Failed to hook GameDefinition::SelectMainGameDefinition");
    shared::hook::HookWrap<shared::raw::Ink::SystemRequestsHandler::StartNewGameFn>(&StartNewGame::m_detourFn)
        .OrDie("Failed to hook SystemRequestsHandler::StartNewGame");
    shared::hook::HookAfter<shared::raw::Telemetry::LoadFactMap>(&LoadFacts::m_detourFn)
        .OrDie("Failed to hook Telemetry::LoadFacts");

    return true;
}

bool DetachHooking()
{
    return shared::hook::Unhook<shared::raw::Telemetry::LoadFactMap>() &&
           shared::hook::Unhook<shared::raw::Ink::SystemRequestsHandler::StartNewGameFn>() &&
           shared::hook::Unhook<shared::raw::GameDefinition::SelectMainGameDefinitionFn>();
}
} // namespace hooking