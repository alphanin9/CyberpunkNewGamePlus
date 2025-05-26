#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"

namespace parser {
	class Parser {
		modsave::SaveHeader m_header;
        std::vector<modsave::NodeEntry> m_flatNodes;
        std::vector<modsave::NodeEntry*> m_nodeList;
		std::vector<std::byte> m_fileStream;

		std::unique_ptr<std::byte[]> m_decompressedDataRaw;
        std::size_t m_decompressedDataSize{};

		void CalculateTrueSizes(std::vector<modsave::NodeEntry*>& aNodes, int aMaxLength) noexcept;
        void FindChildren(modsave::NodeEntry& aNode, int aMaxNextId) noexcept;

		void DecompressFile() noexcept;
		bool LoadNodes() noexcept;
	public:
		bool ParseSavegame(const Red::CString& aSavePath) noexcept;

		modsave::NodeEntry* LookupNode(Red::CName aNodeName) noexcept;

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