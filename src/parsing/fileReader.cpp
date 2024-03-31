#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <print>
#include <tuple>
#include <vector>

#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"
#include "definitions/compression/compressionHeader.hpp"

#include "bufferWriter.hpp"
#include "cursorDef.hpp"

#include "definitions/nodeParsers/parserHelper.hpp"
#include "definitions/nodeParsers/inventory/inventoryNode.hpp"

#include "../dependencies/lz4.h"

// Copypasted from WolvenKit :(
namespace parser {
	std::vector<std::byte> DecompressFile(std::vector<std::byte>& fileStream);

	void loadNodes(std::vector<std::byte>& decompressedData, std::vector<cyberpunk::NodeEntry>& flatNodes);

	void beginParse(std::vector<std::byte>& fileStream) {
		cyberpunk::SaveHeader header;
		{
			auto fileCursorPtr = fileStream.data();
			auto fileCursor = FileCursor{ fileCursorPtr, fileStream.size() };

			const auto magic = fileCursor.readUInt();

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
			auto reverseCursor = FileCursor{ fileStream.data(), fileStream.size() };

			reverseCursor.seekTo(FileCursor::SeekTo::End, -8);

			infoStart = reverseCursor.readInt();
			if (reverseCursor.readUInt() != cyberpunk::FILE_DONE) {
				std::println("DONE check failed");
				return;
			}
		}

		std::println("Info start: {}", infoStart);

		auto fileCursor = FileCursor{ fileStream.data(), fileStream.size() };
		fileCursor.seekTo(FileCursor::SeekTo::Start, infoStart);

		if (fileCursor.readUInt() != cyberpunk::FILE_NODE) {
			std::println("NODE check failed");
			return;
		}

		const auto nodeCount = fileCursor.readVlqInt32();
		std::println("{} nodes", nodeCount);

		std::vector<cyberpunk::NodeEntry> flatNodes{};

		for (auto i = 0; i < nodeCount; i++) {
			flatNodes.push_back(cyberpunk::NodeEntry::fromCursor(fileCursor));
		}

		std::println("Finished reading flat nodes");

		auto decompressedData = DecompressFile(fileStream);

		std::println("Buffer size: {}", decompressedData.size());

		loadNodes(decompressedData, flatNodes);
	}

	std::vector<std::byte> DecompressFile(std::vector<std::byte>& fileStream) {
		// RETARDED CODE ALERT
		// THIS IS COMPLETELY FUCKED
		auto compressionTablePosition = 0ll;
		{
			auto fileCursorPtr = fileStream.data();
			auto fileSize = fileStream.size();
			auto fileCursor = FileCursor{ fileStream.data(), fileStream.size() };
			
			compressionTablePosition = fileCursor.findByteSequence("FZLC");

			std::println("Compression table pos: {}", compressionTablePosition);
		}
		auto decompressionResult = std::vector<std::byte>{};
		{
			auto temporaryResult = std::vector<std::byte>{ fileStream.data(), fileStream.data() + compressionTablePosition };
			auto fileCursor = FileCursor{ fileStream.data(), fileStream.size() };
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

	void findChildren(std::vector<cyberpunk::NodeEntry>& flatNodes, cyberpunk::NodeEntry& node, int maxNextId) {
		if (node.childId > -1) {
			auto nextId = node.nextId;

			if (nextId == -1) {
				nextId = maxNextId;
			}

			for (auto i = node.childId; i < nextId; i++) {
				auto possibleChild = std::find_if(flatNodes.begin(), flatNodes.end(), [i](cyberpunk::NodeEntry& node) {
					return node.id == i;
				});

				if (possibleChild != flatNodes.end()) {
					if (possibleChild->childId > -1) {
						findChildren(flatNodes, *possibleChild, nextId);
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

	void calculateTrueSizes(std::vector<cyberpunk::NodeEntry*>& nodes, int maxLength) {
		for (auto i = 0; i < nodes.size(); i++) {
			cyberpunk::NodeEntry* currentNode = nodes.at(i);
			cyberpunk::NodeEntry* nextNode = nullptr;

			if ((i + 1) < nodes.size()) {
				nextNode = nodes.at(i + 1);
			}

			if (currentNode->nodeChildren.size() > 0) {
				auto& nextChild = currentNode->nodeChildren.front();

				auto blobSize = nextChild->offset - currentNode->offset;
				currentNode->dataSize = blobSize;

				calculateTrueSizes(currentNode->nodeChildren, maxLength);
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
	void loadNodes(std::vector<std::byte>& decompressedData, std::vector<cyberpunk::NodeEntry>& flatNodes) {
		auto cursor = FileCursor{ decompressedData.data(), decompressedData.size() };

		std::println("Assigning nodeids...");

		for (auto& node : flatNodes) {
			cursor.seekTo(FileCursor::SeekTo::Start, node.offset);
			auto nodeId = cursor.readInt();
			node.id = nodeId;
		}

		std::println("Finding children...");

		for (auto& node : flatNodes) {
			if (!node.isChild) {
				findChildren(flatNodes, node, flatNodes.size());
			}
			if (node.nextId > -1) {
				node.nextNode = &*std::find_if(flatNodes.begin(), flatNodes.end(), [&node](cyberpunk::NodeEntry& aNode) {
					return node.nextId == aNode.id;
					});
			}
		}

		std::println("Filtering out children...");

		auto nodeList = std::vector<cyberpunk::NodeEntry*>{};

		for (auto& node : flatNodes) {
			if (!node.isChild) {
				nodeList.push_back(&node);
			}
		}

		std::println("Calculating real sizes...");

		calculateTrueSizes(nodeList, decompressedData.size());

		std::println("Beginning node parsing...");

		for (auto& node : flatNodes) {
			if (!node.isReadByParent) {
				cyberpunk::parseNode(cursor, node);

				const auto readSize = cursor.offset - node.offset;
				auto expectedSize = node.size;

				if (node.isWritingOwnTrailingSize) {
					expectedSize += node.trailingSize;
				}

				if (readSize != expectedSize) {
					// HACK: itemData gets really fucked by this, even on a known good implementation
					if (node.name != L"itemData") {
						std::println("Node {} expected size {} != read size {}", node.id, expectedSize, readSize);
					}
				}
			}
		}

		std::println("Finished parsing nodes...");

		auto inventory = std::find_if(nodeList.begin(), nodeList.end(), [](cyberpunk::NodeEntry* node) {
			return node->name == L"inventory";
		});

		if (inventory == nodeList.end()) {
			return;
		}

		auto inventoryData = reinterpret_cast<cyberpunk::InventoryNode*>((*inventory)->nodeData.get());

		std::println("V's inventories @ {:#010x}...", reinterpret_cast<uintptr_t>(inventoryData));

		auto inventoryNames = std::array<std::tuple<std::uint64_t, std::string>, 5u>{
			std::make_tuple(0x1, "V's inventory"),
			std::make_tuple(0xF4240, "V's car stash"),
			std::make_tuple(0x896368, "Johnny's Inventory"),
			std::make_tuple(9014340, "Kurtz's Inventory"),
			std::make_tuple(0x7901DE03D136A5AF, "V's Wardrobe"),
		};

		for (auto& subInventory : inventoryData->subInventories) {
			auto inventoryName = std::find_if(inventoryNames.begin(), inventoryNames.end(), [&subInventory](std::tuple<std::uint64_t, std::string>& tuple) {
				return std::get<0>(tuple) == subInventory.inventoryId;
			});

			if (inventoryName != inventoryNames.end()) {
				std::println("Inventory \"{}\", {} items", std::get<1>(*inventoryName), subInventory.inventoryItems.size());
			}
			else {
				std::println("Inventory {:#010x}, {} items", subInventory.inventoryId, subInventory.inventoryItems.size());
			}

			for (auto& inventoryItem : subInventory.inventoryItems) {
				std::println("\tItem {:#010x}, qty {}", inventoryItem.itemInfo.itemId.tdbid.value, inventoryItem.itemQuantity);

				if (inventoryItem.itemInfo.itemId.tdbid == RED4ext::TweakDBID{ 95932206476 }) {
					std::println("OMG E3!!!!!!!!!!!!!!!111111111111111111111111");
				}
			}
		}
	}
}