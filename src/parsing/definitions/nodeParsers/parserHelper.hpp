#pragma once

#include <vector>

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

namespace save {
	void ParseNode(FileCursor& cursor, NodeEntry& node);
	void ParseChildren(FileCursor& cursor, std::vector<save::NodeEntry*>& nodeChildren);
}