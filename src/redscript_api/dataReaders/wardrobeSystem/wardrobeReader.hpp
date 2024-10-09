#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <wardrobe/wardrobeSystemNode.hpp>

namespace WardrobeReader
{
class WardrobeResults : public Red::IScriptable
{
public:
    Red::DynArray<Red::ItemID> m_wardrobe{};

    WardrobeResults() = default;
    WardrobeResults(save::WardrobeSystemNode& aNode) noexcept;

    RTTI_IMPL_TYPEINFO(WardrobeResults);
    RTTI_IMPL_ALLOCATOR();
};
}

RTTI_DEFINE_CLASS(WardrobeReader::WardrobeResults, { RTTI_GETTER(m_wardrobe); });