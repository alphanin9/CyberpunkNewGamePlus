#include <memory>

#include "parserHelper.hpp"

#include "defaultNodeData.hpp"
#include "interfaceNodeData.hpp"

#include "inventory/inventoryNode.hpp"
#include "persistency/persistencySystemNode.hpp"
#include "quest/factsDBNode.hpp"
#include "scriptable/scriptableContainerNodeV2.hpp"
#include "stats/statsSystemNode.hpp"
#include "wardrobe/wardrobeSystemNode.hpp"

#include <parsing/cursorDef.hpp>
#include <parsing/definitions/nodeEntry.hpp>

using ParseNodeFn = std::unique_ptr<modsave::NodeDataInterface> (*)(FileCursor& cursor, modsave::NodeEntry& node);

template<typename NodeType>
std::unique_ptr<modsave::NodeDataInterface> GetParserForNode(FileCursor& aCursor, modsave::NodeEntry& aNode)
{
    auto dataPtr = std::make_unique<NodeType>();

    dataPtr->ReadData(aCursor, aNode);

    return dataPtr;
}

#define GET_UNIQUE_NODE_PARSER(nodeTypeName)                                                                           \
    if (aNodeName == modsave::nodeTypeName::m_nodeName)                                                                   \
    {                                                                                                                  \
        return GetParserForNode<modsave::nodeTypeName>;                                                                   \
    }

ParseNodeFn FindParser(Red::CName aNodeName)
{
    using namespace modsave;

    GET_UNIQUE_NODE_PARSER(InventoryNode);
    GET_UNIQUE_NODE_PARSER(ItemDataNode);
    GET_UNIQUE_NODE_PARSER(ScriptableSystemsContainerNodeV2);
    GET_UNIQUE_NODE_PARSER(PersistencySystemNode);
    GET_UNIQUE_NODE_PARSER(FactsDBNode);
    GET_UNIQUE_NODE_PARSER(FactsTableNode);
    GET_UNIQUE_NODE_PARSER(StatsSystemNode);
    GET_UNIQUE_NODE_PARSER(WardrobeSystemNode);

    return GetParserForNode<DefaultNodeData>;
}

void modsave::ParseChildren(FileCursor& cursor, std::vector<modsave::NodeEntry*>& nodeChildren)
{
    for (auto pNode : nodeChildren)
    {
        modsave::ParseNode(cursor, *pNode);
        pNode->isReadByParent = true;
    }
}

void modsave::ParseNode(FileCursor& cursor, modsave::NodeEntry& node)
{
    cursor.seekTo(node.offset);
    cursor.readInt(); // Node ID

    node.nodeData = FindParser(node.m_hash)(cursor, node);
}