#include <RED4ext/RED4ext.hpp>

#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/PhaseInstance.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/NodeDefinition.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/DynamicSpawnSystemNodeDefinition.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/DynamicVehicleSpawn_NodeType.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/DynamicVehicleDespawn_NodeType.hpp>

#include <RedLib.hpp>

#include <context.hpp>
#include "../util/offsetPtr.hpp"

using namespace Red;

namespace test
{
// Dynamic spawn system testing, use at-will quest node execution to spawn random encounters
namespace DynamicSpawnSystemTests
{
using QuestNodeID = uint16_t;
using QuestNodePath = DynArray<QuestNodeID>;
using QuestNodePathHash = uint32_t;

struct QuestPhaseContext
{
    virtual ~QuestPhaseContext() = default;

    uintptr_t m_game;              // 08
    uintptr_t m_unk1;              // 10
    uintptr_t m_unk2;              // 18
    uintptr_t m_unk3;              // 20
    uintptr_t m_unk4;              // 28
    void* m_prefabLoader;        // 30
    uint8_t m_unk38[0x230 - 0x38]; // 38
};
RED4EXT_ASSERT_SIZE(QuestPhaseContext, 0x230);

struct QuestContext
{
    uint8_t m_unk0[0xF8];                         // 00
    DynArray<questPhaseInstance*> m_phaseStack; // F8
    QuestNodeID m_nodeID;                       // 108
    QuestPhaseContext m_phaseContext;           // 110
};
RED4EXT_ASSERT_SIZE(QuestContext, 0x340);
RED4EXT_ASSERT_OFFSET(QuestContext, m_phaseStack, 0xF8);

struct QuestNodeSocket
{
    QuestNodeSocket(CName aName = {})
        : m_name(aName)
        , m_unk08(0)
    {
    }

    CName m_name;
    uint8_t m_unk08;
};

constexpr auto QuestsSystem_CreateContext = 3144298192u;
constexpr auto QuestPhaseInstance_ExecuteNode = 3227858325u;

// Runs a single quest node
// https://github.com/psiberx/cp2077-codeware/blob/7f511885624e7785c8b2311f855883ed4b747fb3/src/App/Quest/QuestPhaseExecutor.hpp#L20
void RunQuestNode(Handle<quest::NodeDefinition> aNode)
{
    auto questsSystem = GetGameSystem<quest::QuestsSystem>();

    auto questMutex = util::OffsetPtr<0x60, SharedSpinLock>::Ptr(questsSystem);
    auto rootPhase = util::OffsetPtr<0x68, Handle<quest::PhaseInstance>>::Ptr(questsSystem);

    std::unique_lock lock(*questMutex);

    using CreateContext_t = void* (*)(quest::QuestsSystem* aThis, QuestContext* aCtx, int a3, int a4, int a5, int a6);

    static const auto s_createContext = UniversalRelocFunc<CreateContext_t>(QuestsSystem_CreateContext);

    QuestContext ctx{};

    s_createContext(questsSystem, &ctx, 1, 0, -1, -1);

    ctx.m_phaseStack.PushBack(rootPhase->GetPtr());

    using ExecuteNode_t = uint8_t (*)(quest::PhaseInstance* aPhase, quest::NodeDefinition* aInputNode, QuestContext& aCtx,
                                    const QuestNodeSocket& aInputSocket, DynArray<QuestNodeSocket>& aOutputSockets);

    static const auto s_executeQuestNode = UniversalRelocFunc<ExecuteNode_t>(QuestPhaseInstance_ExecuteNode);

    QuestNodeSocket socket{};
    DynArray<QuestNodeSocket> outputSockets{};

    s_executeQuestNode(rootPhase->GetPtr(), aNode, ctx, socket, outputSockets);

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
}
}

RTTI_DEFINE_CLASS(test::DynamicSpawnSystemTests::NGPlusDynamicSpawnSystem, {
    RTTI_METHOD(RequestDynamicSpawnSystemSpawn);
    RTTI_METHOD(RequestDynamicSpawnSystemDespawn);
});