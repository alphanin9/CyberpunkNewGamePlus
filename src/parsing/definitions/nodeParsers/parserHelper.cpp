#include <memory>

#include "parserHelper.hpp"

#include "interfaceNodeData.hpp"
#include "defaultNodeData.hpp"

#include "inventory/inventoryNode.hpp"

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

using parseNodeFn = std::unique_ptr<cyberpunk::NodeDataInterface>(*)(FileCursor& cursor, cyberpunk::NodeEntry& node);

std::unique_ptr<cyberpunk::NodeDataInterface> defaultParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::DefaultNodeData>();

	dataPtr->readData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> inventoryParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::InventoryNode>();

	dataPtr->readData(cursor, node);

	return dataPtr;
}

std::unique_ptr<cyberpunk::NodeDataInterface> itemInfoParser(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	auto dataPtr = std::make_unique<cyberpunk::ItemDataNode>();

	dataPtr->readData(cursor, node);

	return dataPtr;
}

parseNodeFn findParser(std::wstring_view nodeName) {
	if (nodeName == cyberpunk::InventoryNode::nodeName) {
		return inventoryParser;
	}
	else if(nodeName == cyberpunk::ItemDataNode::nodeName) {
		return itemInfoParser;
	}

	return defaultParser;
}

void cyberpunk::parseChildren(FileCursor& cursor, std::vector<cyberpunk::NodeEntry*>& nodeChildren) {
	for (auto pNode : nodeChildren) {
		cyberpunk::parseNode(cursor, *pNode);
		pNode->isReadByParent = true;
	}
}

void cyberpunk::parseNode(FileCursor& cursor, cyberpunk::NodeEntry& node) {
	const auto parserFn = findParser(node.name);

	cursor.offset = node.offset;
	cursor.readInt();

	node.nodeData = parserFn(cursor, node);
}