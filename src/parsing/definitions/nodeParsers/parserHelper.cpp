#include <memory>

#include "parserHelper.hpp"

#include "interfaceNodeData.hpp"
#include "defaultNodeData.hpp"

#include "inventory/inventoryNode.hpp"
#include "persistency/persistencySystemNode.hpp"
#include "scriptable/scriptableContainerNode.hpp"

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

using ParseNodeFn = std::unique_ptr<cyberpunk::NodeDataInterface>(*)(FileCursor& cursor, cyberpunk::NodeEntry& node);

std::unique_ptr<cyberpunk::NodeDataInterface> DefaultParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::DefaultNodeData>();

	dataPtr->ReadData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> InventoryParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::InventoryNode>();

	dataPtr->ReadData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> ItemInfoParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::ItemDataNode>();

	dataPtr->ReadData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> ScriptableSystemsContainerParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::ScriptableSystemsContainerNode>();

	dataPtr->ReadData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> PersistencySystemParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::PersistencySystemNode>();

	dataPtr->ReadData(cursor, node);
	
	return dataPtr;
}

ParseNodeFn FindParser(std::wstring_view aNodeName) {
    if (aNodeName == cyberpunk::InventoryNode::nodeName)
    {
		return InventoryParser;
	}
    else if (aNodeName == cyberpunk::ItemDataNode::nodeName)
    {
		return ItemInfoParser;
	}
    else if (aNodeName == cyberpunk::ScriptableSystemsContainerNode::nodeName)
    {
		return ScriptableSystemsContainerParser;
	}
    else if (aNodeName == cyberpunk::PersistencySystemNode::nodeName)
    {
		return PersistencySystemParser;
	}

	return DefaultParser;
}

void cyberpunk::ParseChildren(FileCursor& cursor, std::vector<cyberpunk::NodeEntry*>& nodeChildren) {
	for (auto pNode : nodeChildren) {
		cyberpunk::ParseNode(cursor, *pNode);
		pNode->isReadByParent = true;
	}
}

void cyberpunk::ParseNode(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	const auto parserFn = FindParser(node.name);

	cursor.offset = node.offset; // cursor.seekTo(FileCursor::SeekTo::Start, node.offset);
	cursor.readInt();

	node.nodeData = parserFn(cursor, node);
}