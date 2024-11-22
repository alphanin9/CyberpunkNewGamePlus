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
namespace LoadFacts
{
void OnLoadTelemetryFactMap(HashMap<std::uint32_t, CString>& aMap)
{
    // We add our own facts to the gatherer
    aMap.Insert(FNV1a32("ngplus_active"), "ngplus_active");
    aMap.Insert(FNV1a32("ngplus_q001_start"), "ngplus_q001_start");
    aMap.Insert(FNV1a32("ngplus_standalone_q101_start"), "ngplus_standalone_q101_start");
}
} // namespace LoadFacts
bool InitializeHooking()
{
    shared::hook::HookAfter<shared::raw::Telemetry::LoadFactMap>(&LoadFacts::OnLoadTelemetryFactMap)
        .OrDie("Failed to hook Telemetry::LoadFacts");

    return true;
}

bool DetachHooking()
{
    return shared::hook::Unhook<shared::raw::Telemetry::LoadFactMap>();
}
} // namespace hooking