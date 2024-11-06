#pragma once

#include <detail/hashes.hpp>
#include <util/core.hpp>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace raw
{
namespace Save
{
struct NodeAccessor
{
    RED4ext::BaseStream* m_stream; // 00
    bool m_nodeIsPresentInSave;    // 08, set to 1 if node found I think
    bool unk09;                    // 09, set to a4 in ctor

    static constexpr auto Ctor = util::RawFunc<detail::Hashes::SaveNodeAccessor_ctor,
                                               void* (*)(NodeAccessor* aThis, Red::BaseStream* aStream,
                                                         Red::CName aNodeName, bool aUnk1, bool aIsOptional)>();

    static constexpr auto Dtor = util::RawFunc<detail::Hashes::SaveNodeAccessor_dtor, void* (*)(NodeAccessor* aThis)>();

    static constexpr auto IsGoodNode =
        util::RawFunc<detail::Hashes::SaveNodeAccessor_IsGoodNode, bool (*)(NodeAccessor* aThis)>();

    // Notes: Node name should likely be added to name pool
    NodeAccessor(Red::BaseStream* aStream, Red::CName aNodeName, bool aUnk1, bool aIsOptional)
        : m_stream(nullptr)
        , m_nodeIsPresentInSave(false)
        , unk09(false)
    {
        Ctor(this, aStream, aNodeName, aUnk1, aIsOptional);
    }

    ~NodeAccessor()
    {
        Dtor(this);
    }

    bool IsGood()
    {
        return IsGoodNode(this);
    }
};

RED4EXT_ASSERT_SIZE(NodeAccessor, 0x10);
RED4EXT_ASSERT_OFFSET(NodeAccessor, m_stream, 0x0);
RED4EXT_ASSERT_OFFSET(NodeAccessor, m_nodeIsPresentInSave, 0x8);
RED4EXT_ASSERT_OFFSET(NodeAccessor, unk09, 0x9);

namespace Stream
{
constexpr auto GetSaveVersion = util::RawVFunc<0x48, std::uint32_t (*)(Red::BaseStream*)>();
constexpr auto GetGameVersion = util::RawVFunc<0x50, std::uint32_t (*)(Red::BaseStream*)>();
constexpr auto GetSaveName = util::RawVFunc<0x58, void* (*)(Red::BaseStream*, Red::CString&)>();
constexpr auto Unk90 = util::RawVFunc<0x90, bool (*)(Red::BaseStream*)>();
} // namespace Stream
} // namespace Save
} // namespace raw