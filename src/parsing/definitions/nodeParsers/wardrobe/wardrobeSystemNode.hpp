#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <vector>

#include "../../nodeEntry.hpp"
#include "../interfaceNodeData.hpp"

namespace save
{
struct WardrobeEntry
{
    Red::CName m_appearanceName;
    Red::ItemID m_itemId;

    WardrobeEntry(FileCursor& aCursor);
    WardrobeEntry() = default;
};

class WardrobeSystemNode : public NodeDataInterface
{
    std::vector<WardrobeEntry> m_wardrobeEntries;

public:
    static constexpr auto m_nodeName = "WardrobeSystem";
    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;
    const std::vector<WardrobeEntry>& GetWardrobe() noexcept;
};
} // namespace save

RTTI_DEFINE_CLASS(save::WardrobeEntry, {
    RTTI_PROPERTY(m_appearanceName);
    RTTI_PROPERTY(m_itemId);
});