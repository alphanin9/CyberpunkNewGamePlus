#pragma once
#include <exception>
#include <vector>

#include "../../cursorDef.hpp"

namespace compression {
	constexpr auto COMPRESSION_HEADER_MAGIC = 0x434C5A46u;
	constexpr auto COMPRESSION_BLOCK_MAGIC = 0x584C5A34u;

	struct DataChunkInfo {
		int offset;
		int compressedSize;
		int decompressedSize;

		static DataChunkInfo fromCursor(FileCursor& cursor) {
			DataChunkInfo ret{};

			ret.offset = cursor.readInt();
			ret.compressedSize = cursor.readInt();
			ret.decompressedSize = cursor.readInt();

			return ret;
		}
	};

	struct CompressionHeader {
		std::vector<DataChunkInfo> dataChunkInfo{};

		std::size_t maxEntries;
        std::size_t m_totalChunkSize{};

		static CompressionHeader fromCursor(FileCursor& cursor) {
			const auto compressionHeaderBasePosition = cursor.offset;

			if (cursor.readUInt() != COMPRESSION_HEADER_MAGIC) {
				// Should probably throw an exception here
				return {};
			}

			CompressionHeader ret{};

			auto entryCount = cursor.readInt();

			for (auto i = 0; i < entryCount; i++) {
				ret.dataChunkInfo.push_back(DataChunkInfo::fromCursor(cursor));

				auto& entry = ret.dataChunkInfo.back();

                ret.m_totalChunkSize += std::max(entry.decompressedSize, entry.compressedSize);
			}

			ret.maxEntries = (ret.dataChunkInfo.at(0).offset - (int)(compressionHeaderBasePosition + 8)) / 12;

			cursor.seekTo(FileCursor::SeekTo::Start, ret.dataChunkInfo.at(0).offset);

			return ret;
		}
	};
}