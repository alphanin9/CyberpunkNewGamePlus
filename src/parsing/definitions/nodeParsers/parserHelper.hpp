#pragma once

#include <vector>

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

namespace cyberpunk {
	void ParseNode(FileCursor& cursor, NodeEntry& node);
	void ParseChildren(FileCursor& cursor, std::vector<cyberpunk::NodeEntry*>& nodeChildren);
}