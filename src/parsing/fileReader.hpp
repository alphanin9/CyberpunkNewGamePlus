#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"

namespace parser {
	class Parser {
		cyberpunk::SaveHeader m_header;
		std::vector<cyberpunk::NodeEntry> m_flatNodes;
		std::vector<cyberpunk::NodeEntry*> m_nodeList;
		std::vector<std::byte> m_fileStream;

		void calculateTrueSizes(std::vector<cyberpunk::NodeEntry*>& nodes, int maxLength);
		void findChildren(cyberpunk::NodeEntry& node, int maxNextId);

		std::vector<std::byte> decompressFile(std::vector<std::byte>& fileStream);

		bool loadNodes(std::vector<std::byte>& decompressedData);
	public:
		bool parseMetadata(std::filesystem::path aMetadataPath);
		bool parseSavegame(std::filesystem::path aSavePath);
	};
}