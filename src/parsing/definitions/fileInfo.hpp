#pragma once

#include "../reader/read.hpp"
#include "../cursorDef.hpp"

namespace cyberpunk {
	constexpr auto FILE_MAGIC = 0x43534156; // CSAV
	constexpr auto FILE_DONE = 0x444F4E45; // DONE
	constexpr auto FILE_NODE = 0x4e4f4445; // NODE

	struct SaveHeader {
		std::uint32_t saveVersion;
		std::uint32_t gameVersion;
		std::wstring gameDefinition;
		std::uint64_t timeStamp;
		std::uint32_t archiveVersion;

		static SaveHeader fromCursor(FileCursor fileCursor) {
			SaveHeader header{};
			header.saveVersion = reader::ReadUint(fileCursor);
			header.gameVersion = reader::ReadUint(fileCursor);
			header.gameDefinition = reader::ReadLengthPrefixedString(fileCursor);
			header.timeStamp = reader::ReadUint64(fileCursor);
			header.archiveVersion = reader::ReadUint(fileCursor);

			return header;
		}
	};	
}