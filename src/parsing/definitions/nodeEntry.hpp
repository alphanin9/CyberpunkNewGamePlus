#pragma once
#include "../cursorDef.hpp"
#include "../reader/read.hpp"

#include <string>

namespace cyberpunk {
	struct NodeEntry {
		std::wstring name;
		int nextId;
		int childId;
		int offset;
		int size;

		static NodeEntry fromCursor(FileCursor fileCursor) {
			NodeEntry ret{};

			ret.name = reader::ReadLengthPrefixedString(fileCursor);
			ret.nextId = reader::ReadInt(fileCursor);
			ret.childId = reader::ReadInt(fileCursor);
			ret.offset = reader::ReadInt(fileCursor);
			ret.size = reader::ReadInt(fileCursor);

			return ret;
		}
	};
}
