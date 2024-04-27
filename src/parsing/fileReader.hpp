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

		void CalculateTrueSizes(std::vector<cyberpunk::NodeEntry*>& nodes, int maxLength);
		void FindChildren(cyberpunk::NodeEntry& node, int maxNextId);

		std::vector<std::byte> DecompressFile();
		bool LoadNodes(std::vector<std::byte>& decompressedData);
	public:
		bool ParseMetadata(std::filesystem::path aMetadataPath);
		bool ParseSavegame(std::filesystem::path aSavePath);

		cyberpunk::NodeEntry* LookupNode(std::wstring_view aNodeName);
	};
}