#include <Shared/Util/NamePoolRegistrar.hpp>

#include "Inventory.hpp"

using namespace parser::node;
using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool InventoryNode::OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept
{
    static const auto c_itemDataNodeName = NamePoolRegistrar<"itemData">::Get();

    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (!aStream.IsGood())
    {
        return false;
    }

    return aStream.IsGood();
}

CName InventoryNode::GetName() noexcept
{
    return NamePoolRegistrar<"inventory">::Get();
}

RTTI_DEFINE_CLASS(parser::node::InventoryNode, { RTTI_PARENT(parser::node::SaveNodeData); });