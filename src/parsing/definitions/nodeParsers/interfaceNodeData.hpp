#pragma once
#include "../../cursorDef.hpp"

namespace save {
	struct NodeEntry;

	class NodeDataInterface {
	public:
		virtual void ReadData(FileCursor& cursor, NodeEntry& node) = 0;

		virtual ~NodeDataInterface() {
		
		}
	};
}
