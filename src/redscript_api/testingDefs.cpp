#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/quest/DynamicSpawnSystemNodeDefinition.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/DynamicVehicleDespawn_NodeType.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/DynamicVehicleSpawn_NodeType.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/NodeDefinition.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/PhaseInstance.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>

#include <raw/questsSystem.hpp>

using namespace Red;

namespace test
{
// Dynamic spawn system testing, use at-will quest node execution to spawn random encounters
namespace DynamicSpawnSystemTests
{
// Runs a single quest node
// https://github.com/psiberx/cp2077-codeware/blob/7f511885624e7785c8b2311f855883ed4b747fb3/src/App/Quest/QuestPhaseExecutor.hpp#L20
void RunQuestNode(Handle<quest::NodeDefinition>& aNode)
{
    auto questsSystem = GetGameSystem<quest::QuestsSystem>();

    auto& questMutex = raw::QuestsSystem::QuestMutex::Ref(questsSystem);
    auto& rootPhase = raw::QuestsSystem::RootPhase::Ref(questsSystem);

    std::unique_lock _(questMutex);

    raw::QuestsSystem::QuestContext ctx{};
    raw::QuestsSystem::CreateQuestContext(questsSystem, &ctx, 1, 0, -1, -1);

    ctx.m_phaseStack.PushBack(rootPhase);

    raw::QuestsSystem::QuestNodeSocket socket{};
    DynArray<raw::QuestsSystem::QuestNodeSocket> outputSockets{};

    raw::QuestsSystem::PhaseInstance::RunQuestNode(rootPhase, aNode, ctx, socket, outputSockets);

    ctx.m_phaseStack.Clear();
}

class NGPlusDynamicSpawnSystem : IScriptable
{
public:
    static constexpr CName c_waveTag = "ngplus_debug";

    static void RequestDynamicSpawnSystemSpawn(const DynArray<TweakDBID>& aIds)
    {
        auto node = MakeHandle<quest::DynamicSpawnSystemNodeDefinition>();
        auto nodeType = MakeHandle<quest::DynamicVehicleSpawn_NodeType>();

        nodeType->distanceRange.X = 50;
        nodeType->distanceRange.Y = 50;
        nodeType->spawnDirectionPreference = quest::SpawnDirectionPreference::InFront;
        nodeType->waveTag = c_waveTag;
        nodeType->VehicleData = aIds;

        node->id = 0u;
        node->type = nodeType;

        RunQuestNode(node);
    }

    static void RequestDynamicSpawnSystemDespawn()
    {
        auto node = MakeHandle<quest::DynamicSpawnSystemNodeDefinition>();
        auto nodeType = MakeHandle<quest::DynamicVehicleDespawn_NodeType>();

        nodeType->ImmediateDespawn = true;
        nodeType->waveTag = c_waveTag;

        node->id = 0u;
        node->type = nodeType;

        RunQuestNode(node);
    }
};
} // namespace DynamicSpawnSystemTests
} // namespace test

RTTI_DEFINE_CLASS(test::DynamicSpawnSystemTests::NGPlusDynamicSpawnSystem, {
    RTTI_METHOD(RequestDynamicSpawnSystemSpawn);
    RTTI_METHOD(RequestDynamicSpawnSystemDespawn);
});