#pragma once

// Parsing scriptable systems seems too difficult to be worth it to do it in native code
// So into the trash it goes (for now)
// Someone else can improve it

#include <print>
#include <stdexcept>
#include <vector>
#include <RED4ext/RED4ext.hpp>

#include "../defaultNodeData.hpp"

namespace cyberpunk {
	struct RedPackageHeader {
		std::byte version;
		std::byte unk1;
		std::uint16_t numSections;
		std::uint32_t numComponents;
		std::uint32_t refPoolDescOffset;
		std::uint32_t refPoolDataOffset;
		std::uint32_t namePoolDescOffset;
		std::uint32_t namePoolDataOffset;
		std::uint32_t chunkDescOffset;
		std::uint32_t chunkDataOffset;
	};

	struct RedPackageImportHeader {
		std::uint32_t value;

		static constexpr auto sizeShift = 23;
		static constexpr auto offsetMask = (1u << sizeShift) - 1u;
		static constexpr auto sizeMask = 0xff << sizeShift;
		static constexpr auto syncShift = 31;

		std::uint32_t offset() const {
			return value & offsetMask;
		}

		std::uint8_t size() const {
			return static_cast<std::uint8_t>((value & sizeMask) >> sizeShift);
		}

		bool isSync() const {
			return value >> syncShift;
		}

		RedPackageImportHeader(std::uint32_t bitfield) : value{ bitfield } {

		}

		static RedPackageImportHeader fromCursor(FileCursor& cursor) {
			return RedPackageImportHeader{ cursor.readUInt() };
		}
	};

	struct RedPackageNameHeader {
		std::uint32_t value;

		static constexpr auto sizeShift = 24;
		static constexpr auto offsetMask = 0x00FFFFFF;
		static constexpr auto sizeMask = 0xFF000000;

		std::uint32_t offset() const {
			return value & offsetMask;
		}

		std::uint8_t size() const {
			return static_cast<std::uint8_t>((value & sizeMask) >> sizeShift);
		}

		RedPackageNameHeader(std::uint32_t bitfield) : value{ bitfield } {

		}

		static RedPackageNameHeader fromCursor(FileCursor& cursor) {
			return RedPackageNameHeader{ cursor.readUInt() };
		}
	};

	struct RedImport {
		enum class ImportFlags {
			Default = 0x0, // done / Load from same archive?
			Obligatory = 0x1, // also sync in packages / not used in cr2w currently (1.61)
			Template = 0x2, // done / not used in cr2w currently (1.61)
			Soft = 0x4, // done / Load from any archive?
			Embedded = 0x8, // Load from embedded file
			Inplace = 0x10 // done / not used in cr2w currently (1.61)
		};

		std::uint64_t resourcePathHash;
		std::string importDepotPath;
		ImportFlags importFlags;
	};

	struct RedPackageChunkHeader {
		std::uint32_t typeId;
		std::uint32_t offset;

		static RedPackageChunkHeader fromCursor(FileCursor& cursor) {
			auto ret = RedPackageChunkHeader{};

			ret.typeId = cursor.readUInt();
			ret.offset = cursor.readUInt();

			return ret;
		}
	};

	struct RedPackageFieldHeader {
		std::uint16_t nameId;
		std::uint16_t typeId;
		std::uint32_t offset;

		static RedPackageFieldHeader fromCursor(FileCursor& cursor) {
			auto ret = RedPackageFieldHeader{};

			ret.nameId = cursor.readUShort();
			ret.typeId = cursor.readUShort();
			ret.offset = cursor.readUInt();

			return ret;
		}
	};

	struct RedChunk {
		std::string typeName;
		std::vector<std::tuple<std::string, std::string>> varTypeNamePairs;
		std::vector<std::byte> dataBuffer;
	};

	class ScriptableSystemsContainerNode : public NodeDataInterface {
	public:
		static constexpr std::wstring_view nodeName = L"ScriptableSystemsContainer";

		RedPackageHeader header;
		std::vector<RED4ext::CRUID> rootCruids;
		std::vector<std::string> names;
		std::vector<RedChunk> chunks;

	private:
		RedImport readImport(FileCursor& cursor, RedPackageImportHeader importHeader) {
			RedImport ret{};

			ret.importFlags = importHeader.isSync() ? RedImport::ImportFlags::Obligatory : RedImport::ImportFlags::Default;
			ret.importDepotPath = cursor.readString(importHeader.size());

			return ret;
		}

		void readChunk(FileCursor& cursor, RedChunk& chunk, size_t expectedSizeOfChunk) {
			const auto baseOffset = cursor.offset;
			const auto fieldCount = cursor.readUShort();

			auto fields = std::vector<RedPackageFieldHeader>{};

			for (auto i = 0; i < fieldCount; i++) {
				fields.push_back(RedPackageFieldHeader::fromCursor(cursor));
			}
			
			for (auto& fieldDesc : fields) {
				chunk.varTypeNamePairs.push_back(std::make_tuple(names.at(fieldDesc.typeId), names.at(fieldDesc.nameId)));
			}

			const auto expectedFinish = baseOffset + expectedSizeOfChunk;
			const auto bytesToRead = expectedFinish - cursor.offset;

			chunk.dataBuffer = cursor.readBytes(bytesToRead);

			std::println("Name: {}", chunk.typeName);
			for (auto& fieldDesc : fields) {
				auto typeName = names.at(fieldDesc.typeId);
				auto fieldName = names.at(fieldDesc.nameId);
				std::println("\t{} {} {}", typeName, fieldName, fieldDesc.offset);
			}
		}
	public:
		
		virtual void readData(FileCursor& cursor, NodeEntry& node) {
			const auto dataSize = cursor.readInt();
			auto dataBuffer = cursor.readBytes(dataSize);
			{
				auto cursor = FileCursor{ dataBuffer.data(), dataBuffer.size() };

				if (dataSize == 0) {
					return;
				}

				header.version = static_cast<std::byte>(cursor.readByte());
				header.unk1 = static_cast<std::byte>(cursor.readByte());

				header.numSections = cursor.readUShort();
				header.numComponents = cursor.readUInt();

				if (header.numSections == 7) {
					header.refPoolDescOffset = cursor.readUInt();
					header.refPoolDataOffset = cursor.readUInt();
				}

				header.namePoolDescOffset = cursor.readUInt();
				header.namePoolDataOffset = cursor.readUInt();

				header.chunkDescOffset = cursor.readUInt();
				header.chunkDataOffset = cursor.readUInt();

				const auto numCruids = cursor.readUInt();

				rootCruids.reserve(numCruids);
				for (auto i = 0; i < numCruids; i++) {
					rootCruids.push_back(cursor.readCruid());
				}

				const auto baseOffset = cursor.offset;
				const auto refCount = (header.refPoolDataOffset - header.refPoolDescOffset) / sizeof(RedPackageImportHeader);

				auto refDesc = std::vector<RedPackageImportHeader>{};

				cursor.seekTo(FileCursor::SeekTo::Start, baseOffset + header.refPoolDescOffset);

				for (auto i = 0; i < refCount; i++) {
					refDesc.push_back(RedPackageImportHeader::fromCursor(cursor));
				}

				auto imports = std::vector<RedImport>{};
				imports.reserve(refCount);

				for (auto ref : refDesc) {
					cursor.seekTo(FileCursor::SeekTo::Start, baseOffset + ref.offset());
					imports.push_back(readImport(cursor, ref));
				}

				const auto nameCount = (header.namePoolDataOffset - header.namePoolDescOffset) / sizeof(RedPackageNameHeader);
				cursor.seekTo(FileCursor::SeekTo::Start, baseOffset + header.namePoolDescOffset);
				auto nameDesc = std::vector<RedPackageNameHeader>{};
				nameDesc.reserve(nameCount);

				for (auto i = 0; i < nameCount; i++) {
					nameDesc.push_back(RedPackageNameHeader::fromCursor(cursor));
				}

				for (auto name : nameDesc) {
					cursor.seekTo(FileCursor::SeekTo::Start, baseOffset + name.offset());
					names.push_back(cursor.readNullTerminatedString());
				}

				const auto chunkCount = (header.chunkDataOffset - header.chunkDescOffset) / sizeof(RedPackageChunkHeader);
				cursor.seekTo(FileCursor::SeekTo::Start, baseOffset + header.chunkDescOffset);

				auto chunkHeaders = std::vector<RedPackageChunkHeader>{};

				for (auto i = 0; i < chunkCount; i++) {
					chunkHeaders.push_back(RedPackageChunkHeader::fromCursor(cursor));
				}

				const auto chunkPos = cursor.offset;
				
				for (auto chunkHeader : chunkHeaders) {
					auto chunk = RedChunk{};
					chunk.typeName = names.at(chunkHeader.typeId);
					
					chunks.push_back(chunk);
				}

				cursor.seekTo(FileCursor::SeekTo::Start, chunkPos); // Makes no sense, given we don't read anything
				// But do as Seberoth does

				for (auto i = 0; i < chunks.size(); i++) {
					auto& chunk = chunks.at(i);
					auto chunkSize = 0;

					if (i == chunks.size() - 1) {
						chunkSize = cursor.size - cursor.offset;
					}
					else {
						chunkSize = (baseOffset + chunkHeaders.at(i + 1).offset) - cursor.offset;
					}

					readChunk(cursor, chunk, chunkSize);
				}
			}
		}
	};
}