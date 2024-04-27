#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <print>
#include <tuple>
#include <vector>

#include "fileReader.hpp"

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"
#include "definitions/compression/compressionHeader.hpp"

#include "bufferWriter.hpp"
#include "cursorDef.hpp"

#include "definitions/nodeParsers/parserHelper.hpp"
#include "definitions/nodeParsers/inventory/inventoryNode.hpp"

#include "../dependencies/lz4.h"

#include <nlohmann/json.hpp>
#include <RedLib.hpp>

#include "../context/context.hpp"

// Copypasted from WolvenKit :(
namespace parser {
	bool Parser::ParseMetadata(std::filesystem::path aMetadataPath) {
		return true;
	}

	bool Parser::ParseSavegame(std::filesystem::path aSavePath) {

		const auto bufferSize = std::filesystem::file_size(aSavePath);

		m_fileStream = std::vector<std::byte>{ bufferSize };
		{
			auto file = std::ifstream{ aSavePath, std::ios_base::binary };
			file.read(reinterpret_cast<char*>(m_fileStream.data()), bufferSize);
			// std::println("Read file!");
		}

		{
			auto fileCursor = FileCursor{ m_fileStream.data(), m_fileStream.size() };

			const auto magic = fileCursor.readUInt();

			if (magic != cyberpunk::FILE_MAGIC) {
				return false;
			}

			m_header = cyberpunk::SaveHeader::fromCursor(fileCursor);
		}

		if (m_header.gameVersion < 2000) {
			return false;
		}
		
		auto infoStart = 0;
		{
			auto reverseCursorPtr = m_fileStream.data() + m_fileStream.size() - 8u;
			auto reverseCursor = FileCursor{ m_fileStream.data(), m_fileStream.size() };

			reverseCursor.seekTo(FileCursor::SeekTo::End, -8);

			infoStart = reverseCursor.readInt();
			if (reverseCursor.readUInt() != cyberpunk::FILE_DONE) {
				return false;
			}
		}

		// std::println("Info start: {}", infoStart);

		auto fileCursor = FileCursor{ m_fileStream.data(), m_fileStream.size() };
		fileCursor.seekTo(FileCursor::SeekTo::Start, infoStart);

		if (fileCursor.readUInt() != cyberpunk::FILE_NODE) {
			return false;
		}

		const auto nodeCount = fileCursor.readVlqInt32();

		for (auto i = 0; i < nodeCount; i++) {
			m_flatNodes.push_back(cyberpunk::NodeEntry::fromCursor(fileCursor));
		}

		// std::println("Finished reading flat nodes");

		auto decompressedData = DecompressFile();

		// std::println("Buffer size: {}", decompressedData.size());

		return LoadNodes(decompressedData);
	}

	void DumpItemInfo(cyberpunk::ItemInfo& aItemInfo, int aIndentation) {
        std::string padding{};
        for (auto i = 0; i < aIndentation; i++)
        {
            padding.push_back(' ');
        }

		Red::CString str;
		Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aItemInfo.itemId.tdbid);
		
		PluginContext::Spew(std::format("{}{}", padding, str.c_str()));
	}

	void DumpItemSlotParts(cyberpunk::ItemSlotPart& aItemSlotPart, int aIndentation) {
		if (!aItemSlotPart.isValid) {
			return;
		}
        std::string padding{};

		for (auto i = 0; i < aIndentation; i++)
        {
            padding.push_back(' ');
		}

		Red::CString str;
		Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aItemSlotPart.attachmentSlotTdbId);

		PluginContext::Spew(std::format("{}Attachment slot {}", padding, str.c_str()));
        DumpItemInfo(aItemSlotPart.itemInfo, aIndentation + 1);
		for (auto& i : aItemSlotPart.children) {
			DumpItemSlotParts(i, aIndentation + 2);
		}
	}

	void DumpItem(cyberpunk::ItemData& aItemData) {
		Red::CString str;
		Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aItemData.itemInfo.itemId.tdbid);

		const auto itemQuantity = aItemData.hasQuantity() ? aItemData.itemQuantity : 1;

		PluginContext::Spew(std::format("{}, qty {}", str.c_str(), itemQuantity));
		
		if (aItemData.hasExtendedData()) {
			DumpItemSlotParts(aItemData.itemSlotPart, 1);
		}
	}

	std::vector<std::byte> Parser::DecompressFile() {
		// RETARDED CODE ALERT
		// THIS IS COMPLETELY FUCKED
		auto compressionTablePosition = 0ll;
		{
			auto fileCursorPtr = m_fileStream.data();
			auto fileSize = m_fileStream.size();
			auto fileCursor = FileCursor{ m_fileStream.data(), m_fileStream.size() };
			
			compressionTablePosition = fileCursor.findByteSequence("FZLC");

			// std::println("Compression table pos: {}", compressionTablePosition);
		}
		auto decompressionResult = std::vector<std::byte>{};
		{
			auto temporaryResult = std::vector<std::byte>{ m_fileStream.data(), m_fileStream.data() + compressionTablePosition };
			auto fileCursor = FileCursor{ m_fileStream.data(), m_fileStream.size() };
			fileCursor.seekTo(FileCursor::SeekTo::Start, compressionTablePosition);

			const auto compressionHeader = compression::CompressionHeader::fromCursor(fileCursor);

			const auto tableEntriesCount = compressionHeader.maxEntries;
			const auto chunkSize = tableEntriesCount == 0x100 ? 0x00040000 : 0x00080000;
			{
				auto decompressionBuffer = std::vector<std::byte>{ };

				for (auto& chunkInfo : compressionHeader.dataChunkInfo) {
					assert(fileCursor.offset == chunkInfo.offset);

					auto outBuffer = std::vector<std::byte>{};

					if (fileCursor.readUInt() == compression::COMPRESSION_BLOCK_MAGIC) {
						fileCursor.readInt();

						auto inBuffer = fileCursor.readBytes(chunkInfo.compressedSize - 8);
						// RETARDED CODE
						// NEEDS TO HAVE A BETTER, FASTER WAY
						outBuffer.resize(chunkInfo.decompressedSize);

						LZ4_decompress_safe(reinterpret_cast<char*>(inBuffer.data()), reinterpret_cast<char*>(outBuffer.data()), inBuffer.size(), outBuffer.size());
					}
					else {
						fileCursor.offset -= 4;

						outBuffer = fileCursor.readBytes(chunkInfo.compressedSize);
					}

					decompressionBuffer.insert(decompressionBuffer.end(), outBuffer.begin(), outBuffer.end());
				}

				const auto uncompressedData = [&temporaryResult, &decompressionBuffer, &compressionHeader, tableEntriesCount, chunkSize]() {
					const auto startPos = temporaryResult.size();

					const auto chunkCount = decompressionBuffer.size() / chunkSize;
					const auto chunkBytesLeft = decompressionBuffer.size() % chunkSize;

					auto chunks = std::vector<compression::DataChunkInfo>();

					auto processedBuffer = std::vector<std::byte>{};
					{
						auto inBuffer = std::vector<std::byte>{};

						auto index = 0;
						for (; index < chunkCount; index++) {
							inBuffer.clear();
							inBuffer.resize(chunkSize);

							std::copy(&decompressionBuffer.at(index * chunkSize), &decompressionBuffer.at(index * chunkSize + inBuffer.size()), inBuffer.begin());

							compression::DataChunkInfo chunk{};

							chunk.offset = processedBuffer.size();
							chunk.compressedSize = inBuffer.size();
							chunk.decompressedSize = inBuffer.size();

							processedBuffer.insert(processedBuffer.end(), inBuffer.begin(), inBuffer.end());
							chunks.push_back(chunk);
						}

						// Process leftover size chunk
						compression::DataChunkInfo chunk{};

						chunk.offset = processedBuffer.size();
						chunk.compressedSize = chunkBytesLeft;
						chunk.decompressedSize = chunkBytesLeft;

						processedBuffer.insert(processedBuffer.end(), std::next(decompressionBuffer.begin(), processedBuffer.size()), decompressionBuffer.end());

						chunks.push_back(chunk);
					}
					const auto dataOffset = startPos + 8 + (tableEntriesCount * 12);

					for (auto& chunkInfo : chunks) {
						chunkInfo.offset += dataOffset;
					}

					auto retBuffer = std::vector<std::byte>{};
					{
						auto compressionHeaderWriter = BufferWriter{};

						compressionHeaderWriter.writeUInt(compression::COMPRESSION_HEADER_MAGIC);
						compressionHeaderWriter.writeInt(chunks.size());

						for (auto& chunkInfo : chunks) {
							compressionHeaderWriter.writeInt(chunkInfo.offset);
							compressionHeaderWriter.writeInt(chunkInfo.compressedSize);
							compressionHeaderWriter.writeInt(chunkInfo.decompressedSize);
						}

						compressionHeaderWriter.writeEmpty((tableEntriesCount - chunks.size()) * 12);

						retBuffer.insert(retBuffer.end(), compressionHeaderWriter.buffer.begin(), compressionHeaderWriter.buffer.end());
					}
					
					retBuffer.insert(retBuffer.end(), processedBuffer.begin(), processedBuffer.end());

					return retBuffer;
				}();

				decompressionResult = temporaryResult;
				decompressionResult.insert(decompressionResult.end(), uncompressedData.begin(), uncompressedData.end());
			}
		}
		
		return decompressionResult;
	}

	void Parser::FindChildren(cyberpunk::NodeEntry& node, int maxNextId) {
		if (node.childId > -1) {
			auto nextId = node.nextId;

			if (nextId == -1) {
				nextId = maxNextId;
			}

			for (auto i = node.childId; i < nextId; i++) {
				auto possibleChild = std::find_if(m_flatNodes.begin(), m_flatNodes.end(), [i](cyberpunk::NodeEntry& node) {
					return node.id == i;
				});

				if (possibleChild != m_flatNodes.end()) {
					if (possibleChild->childId > -1) {
						FindChildren(*possibleChild, nextId);
						node.addChild(&*possibleChild);
					}
					else {
						if (!possibleChild->isChild) {
							node.addChild(&*possibleChild);
						}
					}
				}
				
			}
		}
	}

	void Parser::CalculateTrueSizes(std::vector<cyberpunk::NodeEntry*>& nodes, int maxLength) {
		for (auto i = 0ull; i < nodes.size(); i++) {
			cyberpunk::NodeEntry* currentNode = nodes.at(i);
			cyberpunk::NodeEntry* nextNode = nullptr;

			if ((i + 1) < nodes.size()) {
				nextNode = nodes.at(i + 1);
			}

			if (currentNode->nodeChildren.size() > 0) {
				auto& nextChild = currentNode->nodeChildren.front();

				auto blobSize = nextChild->offset - currentNode->offset;
				currentNode->dataSize = blobSize;

				CalculateTrueSizes(currentNode->nodeChildren, maxLength);
			}
			else {
				currentNode->dataSize = currentNode->size;
			}

			if (nextNode) {
				auto blobSize = nextNode->offset - (currentNode->offset + currentNode->size);
				currentNode->trailingSize = blobSize;
			}
			else {
				if (!currentNode->parent) {
					auto lastNodeEnd = currentNode->offset + currentNode->size;

					if (lastNodeEnd < maxLength) {
						currentNode->trailingSize = maxLength - lastNodeEnd;
					}

					continue;
				}
				
				auto parentNode = currentNode->parent;

				auto nextToParentNodeIter = parentNode->nextNode;

				// Something WKit does due to:
				// This is the last child on the last node. The next valid offset would be the end of the data
				// Create a virtual node for this so the code below can grab the offset
				auto nextToParentNodeOffset = maxLength;

				if (nextToParentNodeIter) {
					nextToParentNodeOffset = nextToParentNodeIter->offset;
				}

				auto parentMax = parentNode->offset + parentNode->size;
				auto childMax = currentNode->offset + currentNode->size;

				auto blobSize = nextToParentNodeOffset - childMax;

				if (parentMax > childMax) {
					currentNode->trailingSize = blobSize;
				}
				else if (parentMax == childMax) {
					parentNode->trailingSize = blobSize;
				}
			}
		}
	}

	// The most reasonable course of action is keeping flatNodes allocated as long as possible, and making the actual node list hold ptrs to nodes in flatNodes
	bool Parser::LoadNodes(std::vector<std::byte>& decompressedData) {
		auto cursor = FileCursor{ decompressedData.data(), decompressedData.size() };

		//std::println("Assigning nodeids...");

		for (auto& node : m_flatNodes) {
			cursor.seekTo(FileCursor::SeekTo::Start, node.offset);
			auto nodeId = cursor.readInt();
			node.id = nodeId;
		}

		//std::println("Finding children...");

		for (auto& node : m_flatNodes) {
			if (!node.isChild) {
				FindChildren(node, m_flatNodes.size());
			}
			if (node.nextId > -1) {
				node.nextNode = &*std::find_if(m_flatNodes.begin(), m_flatNodes.end(), [&node](cyberpunk::NodeEntry& aNode) {
					return node.nextId == aNode.id;
					});
			}
		}

		//std::println("Filtering out children...");

		for (auto& node : m_flatNodes) {
			if (!node.isChild) {
				m_nodeList.push_back(&node);
			}
		}

		//std::println("Calculating real sizes...");

		CalculateTrueSizes(m_nodeList, decompressedData.size());

		//std::println("Beginning node parsing...");

		for (auto& node : m_flatNodes) {
			if (!node.isReadByParent) {
				cyberpunk::ParseNode(cursor, node);

				const auto readSize = cursor.offset - node.offset;
				auto expectedSize = node.size;

				if (node.isWritingOwnTrailingSize) {
					expectedSize += node.trailingSize;
				}

				if (readSize != expectedSize) {
					// HACK: itemData gets really fucked by this, even on a known good implementation
					if (node.name != L"itemData") {
						PluginContext::Error(std::format("Node {} expected size {} != read size {}", node.id, expectedSize, readSize));
					}
				}
			}
		}

		constexpr auto shouldDumpInventory = false;

		if constexpr (shouldDumpInventory) {
			auto inventory = LookupNode(L"inventory");
			auto inventoryData = reinterpret_cast<cyberpunk::InventoryNode*>(inventory->nodeData.get());

			for (auto& subInventory : inventoryData->subInventories) {
                PluginContext::Spew(std::format("Inventory {}", subInventory.inventoryId));
				for (auto& inventoryItem : subInventory.inventoryItems) {
					DumpItem(inventoryItem);
				}
			}
		}
		
		return true;
	}

	cyberpunk::NodeEntry* Parser::LookupNode(std::wstring_view aNodeName) {
		auto node = std::find_if(m_nodeList.begin(), m_nodeList.end(), [aNodeName](const cyberpunk::NodeEntry* aNode) {
			return aNode->name == aNodeName;
		});

		if (node == m_nodeList.end())
        {
            throw std::runtime_error{"Failed to find node!"};
		}

		return *node;
	}
}