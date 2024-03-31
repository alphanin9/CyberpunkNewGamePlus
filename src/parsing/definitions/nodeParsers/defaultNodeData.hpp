#pragma once
#include "interfaceNodeData.hpp"
#include "parserHelper.hpp"

namespace cyberpunk {
	// Destructors seem kinda wonky with the whole polymorphism thing
	// But this should be fine, seeing as inner struct destructors get called with only the interface's destructor being defines
	class DefaultNodeData : public NodeDataInterface {
	public:
		std::vector<std::byte> headingBlob;
		std::vector<std::byte> trailingBlob;

		virtual void readData(FileCursor& cursor, NodeEntry& node) {
			headingBlob = cursor.readBytes(node.dataSize - 4);
			cyberpunk::parseChildren(cursor, node.nodeChildren);
			trailingBlob = cursor.readBytes(node.trailingSize);
		}
	};
}