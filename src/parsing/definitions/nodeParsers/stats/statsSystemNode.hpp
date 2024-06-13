#pragma once
#include "../interfaceNodeData.hpp"
#include "../../nodeEntry.hpp"

namespace cyberpunk
{
class StatsSystemNode : public NodeDataInterface
{
public:
    static constexpr Red::CName m_nodeName = "StatsSystem";

    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;
};
}