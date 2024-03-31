#pragma once

#include <vector>

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

namespace cyberpunk {
	void parseNode(FileCursor& cursor, NodeEntry& node);
	void parseChildren(FileCursor& cursor, std::vector<cyberpunk::NodeEntry*>& nodeChildren);
}