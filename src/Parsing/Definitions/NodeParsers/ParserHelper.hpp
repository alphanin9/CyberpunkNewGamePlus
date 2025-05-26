#pragma once

#include <vector>

#include "../nodeEntry.hpp"
#include "../../cursorDef.hpp"

namespace modsave {
	void ParseNode(FileCursor& cursor, NodeEntry& node);
	void ParseChildren(FileCursor& cursor, std::vector<modsave::NodeEntry*>& nodeChildren);
}