#include <fstream>
#include <print>

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"

#include "reader/read.hpp"

#include "cursorDef.hpp"
namespace parser {
	void DecompressFile(std::vector<std::byte>& fileStream);

	void beginParse(std::vector<std::byte>& fileStream) {
		cyberpunk::SaveHeader header;
		{
			auto fileCursorPtr = fileStream.data();
			auto fileCursor = FileCursor{ &fileCursorPtr };

			const auto magic = reader::ReadUint(fileCursor);

			if (magic != cyberpunk::FILE_MAGIC) {
				std::println("Magic check failed, {}", magic);
				return;
			}

			std::println("Magic checks out");
			header = cyberpunk::SaveHeader::fromCursor(fileCursor);
		}
		
		auto infoStart = 0;
		{
			auto reverseCursorPtr = fileStream.data() + fileStream.size() - 8u;
			auto reverseCursor = FileCursor{ &reverseCursorPtr };

			infoStart = reader::ReadInt(reverseCursor);
			if (reader::ReadUint(reverseCursor) != cyberpunk::FILE_DONE) {
				std::println("DONE check failed");
				return;
			}
		}

		std::println("Info start: {}", infoStart);
		auto fileCursorPtr = fileStream.data() + infoStart;
		auto fileCursor = FileCursor{ &fileCursorPtr };

		if (reader::ReadUint(fileCursor) != cyberpunk::FILE_NODE) {
			std::println("NODE check failed");
			return;
		}

		const auto nodeCount = reader::ReadVlqInt32(fileCursor);
		std::println("{} nodes", nodeCount);

		std::vector<cyberpunk::NodeEntry> flatNodes{};

		for (auto i = 0; i < nodeCount; i++) {
			flatNodes.push_back(cyberpunk::NodeEntry::fromCursor(fileCursor));
		}

		std::println("Finished reading flat nodes");

		DecompressFile(fileStream);
	}

	void DecompressFile(std::vector<std::byte>& fileStream) {
		{
			auto fileCursorPtr = fileStream.data();
			auto fileSize = fileStream.size();
			auto fileCursor = FileCursor{ &fileCursorPtr };
			
			auto compressionTablePosition = reader::FindByteSequence(fileCursor, fileSize, "FZLC");

			std::println("Compression table pos: {}", compressionTablePosition);
		}
	}
}