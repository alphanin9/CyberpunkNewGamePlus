#include <memory>

#include "parserHelper.hpp"

#include "interfaceNodeData.hpp"
#include "defaultNodeData.hpp"

#include "inventory/inventoryNode.hpp"
#include "persistency/persistencySystemNode.hpp"
#include "quest/factsDBNode.hpp"
#include "scriptable/scriptableContainerNode.hpp"
#include "stats/statsSystemNode.hpp"

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

using ParseNodeFn = std::unique_ptr<cyberpunk::NodeDataInterface>(*)(FileCursor& cursor, cyberpunk::NodeEntry& node);

template<typename NodeType>
std::unique_ptr<cyberpunk::NodeDataInterface> GetParserForNode(FileCursor& aCursor, cyberpunk::NodeEntry& aNode)
{
    auto dataPtr = std::make_unique<NodeType>();

	dataPtr->ReadData(aCursor, aNode);

	return dataPtr;
}

#define GET_UNIQUE_NODE_PARSER(nodeTypeName)                                                                           \
    if (aNodeName == cyberpunk::nodeTypeName::m_nodeName)                                                              \
    {                                                                                                                  \
		return GetParserForNode<cyberpunk::nodeTypeName>;																\
	}																													\

ParseNodeFn FindParser(Red::CName aNodeName)
{
	using namespace cyberpunk;

	GET_UNIQUE_NODE_PARSER(InventoryNode);
    GET_UNIQUE_NODE_PARSER(ItemDataNode);
    GET_UNIQUE_NODE_PARSER(ScriptableSystemsContainerNode);
    GET_UNIQUE_NODE_PARSER(PersistencySystemNode);
    GET_UNIQUE_NODE_PARSER(FactsDBNode);
    GET_UNIQUE_NODE_PARSER(FactsTableNode);
	// Disabled ATM due to issues with DataBuffer, waiting for Psiberx...
    GET_UNIQUE_NODE_PARSER(StatsSystemNode); 

	return GetParserForNode<DefaultNodeData>;
}

void cyberpunk::ParseChildren(FileCursor& cursor, std::vector<cyberpunk::NodeEntry*>& nodeChildren) {
	for (auto pNode : nodeChildren) {
		cyberpunk::ParseNode(cursor, *pNode);
		pNode->isReadByParent = true;
	}
}

void cyberpunk::ParseNode(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	cursor.seekTo(node.offset);
	cursor.readInt(); // Node ID

	node.nodeData = FindParser(node.m_hash)(cursor, node);
}