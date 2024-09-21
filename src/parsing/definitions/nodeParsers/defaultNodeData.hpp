#pragma once
#include "interfaceNodeData.hpp"
#include "parserHelper.hpp"

namespace save {
	// Destructors seem kinda wonky with the whole polymorphism thing
	// But this should be fine, seeing as inner struct destructors get called with only the interface's destructor being defined
	class DefaultNodeData : public NodeDataInterface {
	public:
		//std::vector<std::byte> headingBlob;
		//std::vector<std::byte> trailingBlob;

		virtual void ReadData(FileCursor& cursor, NodeEntry& node) {
			// headingBlob = cursor.readBytes(node.dataSize - 4);
            cursor.offset += node.dataSize - 4;
			save::ParseChildren(cursor, node.nodeChildren);
            cursor.offset += node.trailingSize;
			//trailingBlob = cursor.readBytes(node.trailingSize);
		}
	};
}