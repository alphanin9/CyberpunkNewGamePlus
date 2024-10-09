#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"

namespace parser {
	class Parser {
		save::SaveHeader m_header;
		std::vector<save::NodeEntry> m_flatNodes;
		std::vector<save::NodeEntry*> m_nodeList;
		std::vector<std::byte> m_fileStream;

		std::unique_ptr<std::byte[]> m_decompressedDataRaw;
        std::size_t m_decompressedDataSize{};

		void CalculateTrueSizes(std::vector<save::NodeEntry*>& nodes, int maxLength) noexcept;
		void FindChildren(save::NodeEntry& node, int maxNextId) noexcept;

		void DecompressFile() noexcept;
		bool LoadNodes() noexcept;
	public:
		bool ParseSavegame(const Red::CString& aSavePath) noexcept;

		save::NodeEntry* LookupNode(Red::CName aNodeName) noexcept;

		template<typename NodeType>
        inline NodeType* LookupNodeData() noexcept
        {
			auto nodePtr = LookupNode(NodeType::m_nodeName);

			if (!nodePtr)
            {
                return nullptr;
			}

			return static_cast<NodeType*>(nodePtr->nodeData.get());
		}
	};
}