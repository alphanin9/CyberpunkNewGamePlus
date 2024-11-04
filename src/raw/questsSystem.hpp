#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/quest/NodeDefinition.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/PhaseInstance.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>

#include <detail/hashes.hpp>
#include <util/core.hpp>

// https://github.com/psiberx/cp2077-archive-xl/blob/main/src/Red/QuestsSystem.hpp
namespace raw
{
namespace Facts
{
struct FactKey
{
    std::uint32_t m_hash{};

    constexpr FactKey(const char* aStr) noexcept
        : m_hash(Red::FNV1a32(aStr))
    {
       
    }

    operator std::uint32_t() const noexcept
    {
        return m_hash;
    }
};

struct FactStore
{
    std::uint64_t unk00;
    Red::Map<FactKey, int> data;
};

struct FactsDBManager
{
    static constexpr auto c_namedFacts = 1u;
    static constexpr auto c_maxFactsTables = 11u;

    virtual ~FactsDBManager() = 0;                                                 // 00
    virtual void sub_08() = 0;                                                     // 08
    virtual int32_t GetFact(std::uint32_t aStore, FactKey aFact) = 0;              // 10
    virtual void SetFact(std::uint32_t aStore, FactKey aFact, int aValue) = 0; // 18

    inline int GetFact(FactKey aFact)
    {
        return GetFact(c_namedFacts, aFact);
    }

    inline void SetFact(FactKey aFact, int aValue)
    {
        SetFact(c_namedFacts, aFact, aValue);
    }

    FactStore* m_factStorage[c_maxFactsTables];
};
}

namespace QuestsSystem
{
// Vtable index 62
// Sig: 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 ED B8
constexpr auto AddQuest =
    util::RawFunc<detail::Hashes::QuestsSystem_AddQuest, void (*)(Red::quest::QuestsSystem*, Red::ResourcePath)>();

using QuestMutex = util::OffsetPtr<0x60, Red::SharedSpinLock>;
using RootPhase = util::OffsetPtr<0x68, Red::Handle<Red::quest::PhaseInstance>>;
using QuestsList = util::OffsetPtr<0xA8, Red::DynArray<Red::ResourcePath>>;
using FactsDB = util::OffsetPtr<0xF8, Facts::FactsDBManager*>;

using QuestNodeID = uint16_t;
using QuestNodePath = Red::DynArray<QuestNodeID>;
using QuestNodePathHash = uint32_t;

struct QuestPhaseContext
{
    virtual ~QuestPhaseContext() = default;

    uintptr_t m_game;              // 08
    uintptr_t m_unk1;              // 10
    uintptr_t m_unk2;              // 18
    uintptr_t m_unk3;              // 20
    uintptr_t m_unk4;              // 28
    void* m_prefabLoader;          // 30
    uint8_t m_unk38[0x230 - 0x38]; // 38
};
RED4EXT_ASSERT_SIZE(QuestPhaseContext, 0x230);

struct QuestContext
{
    uint8_t m_unk0[0xF8];                                   // 00
    Red::DynArray<Red::quest::PhaseInstance*> m_phaseStack; // F8
    QuestNodeID m_nodeID;                                   // 108
    QuestPhaseContext m_phaseContext;                       // 110
};
RED4EXT_ASSERT_SIZE(QuestContext, 0x340);
RED4EXT_ASSERT_OFFSET(QuestContext, m_phaseStack, 0xF8);

struct QuestNodeSocket
{
    QuestNodeSocket(Red::CName aName = {})
        : m_name(aName)
        , m_unk08(0)
    {
    }

    Red::CName m_name;
    uint8_t m_unk08;
};

constexpr auto CreateQuestContext =
    util::RawFunc<detail::Hashes::QuestsSystem_CreateContext,
                  void* (*)(Red::quest::QuestsSystem*, QuestContext*, int, int, int, int)>();

namespace PhaseInstance
{
constexpr auto RunQuestNode =
    util::RawFunc<detail::Hashes::QuestPhaseInstance_ExecuteNode,
                  uint8_t (*)(Red::quest::PhaseInstance*, Red::quest::NodeDefinition*, QuestContext&,
                              const QuestNodeSocket&, Red::DynArray<QuestNodeSocket>&)>();
}
}
}