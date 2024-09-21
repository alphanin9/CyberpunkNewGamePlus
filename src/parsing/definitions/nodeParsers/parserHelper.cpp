#include <memory>

#include "parserHelper.hpp"

#include "defaultNodeData.hpp"
#include "interfaceNodeData.hpp"

#include "inventory/inventoryNode.hpp"
#include "persistency/persistencySystemNode.hpp"
#include "quest/factsDBNode.hpp"
#include "scriptable/scriptableContainerNode.hpp"
#include "stats/statsSystemNode.hpp"
#include "wardrobe/wardrobeSystemNode.hpp"

#include "../../cursorDef.hpp"
#include "../nodeEntry.hpp"

using ParseNodeFn = std::unique_ptr<save::NodeDataInterface> (*)(FileCursor& cursor, save::NodeEntry& node);

template<typename NodeType>
std::unique_ptr<save::NodeDataInterface> GetParserForNode(FileCursor& aCursor, save::NodeEntry& aNode)
{
    auto dataPtr = std::make_unique<NodeType>();

    dataPtr->ReadData(aCursor, aNode);

    return dataPtr;
}

#define GET_UNIQUE_NODE_PARSER(nodeTypeName)                                                                           \
    if (aNodeName == save::nodeTypeName::m_nodeName)                                                                   \
    {                                                                                                                  \
        return GetParserForNode<save::nodeTypeName>;                                                                   \
    }

ParseNodeFn FindParser(Red::CName aNodeName)
{
    using namespace save;

    GET_UNIQUE_NODE_PARSER(InventoryNode);
    GET_UNIQUE_NODE_PARSER(ItemDataNode);
    GET_UNIQUE_NODE_PARSER(ScriptableSystemsContainerNode);
    GET_UNIQUE_NODE_PARSER(PersistencySystemNode);
    GET_UNIQUE_NODE_PARSER(FactsDBNode);
    GET_UNIQUE_NODE_PARSER(FactsTableNode);
    GET_UNIQUE_NODE_PARSER(StatsSystemNode);
    GET_UNIQUE_NODE_PARSER(WardrobeSystemNode);

    return GetParserForNode<DefaultNodeData>;
}

void save::ParseChildren(FileCursor& cursor, std::vector<save::NodeEntry*>& nodeChildren)
{
    for (auto pNode : nodeChildren)
    {
        save::ParseNode(cursor, *pNode);
        pNode->isReadByParent = true;
    }
}

void save::ParseNode(FileCursor& cursor, save::NodeEntry& node)
{
    cursor.seekTo(node.offset);
    cursor.readInt(); // Node ID

    node.nodeData = FindParser(node.m_hash)(cursor, node);
}