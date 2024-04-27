#pragma once
#include "../../cursorDef.hpp"

namespace cyberpunk {
	struct NodeEntry;

	class NodeDataInterface {
	public:
		virtual void ReadData(FileCursor& cursor, NodeEntry& node) = 0;

		virtual ~NodeDataInterface() {
		
		}
	};
}
