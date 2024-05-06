#pragma once

#include <format>
#include <fstream>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Package.hpp>
#include <RedLib.hpp>

#include "helpers/rttiClassCreator.hpp"
#include "helpers/nativeScriptableReader.hpp"
#include "helpers/classDefinitions/playerDevelopmentData.hpp"
#include "../defaultNodeData.hpp"

namespace cyberpunk {
	struct RedPackageHeader {
		std::byte m_version;
		std::byte m_unk1;
		std::uint16_t m_numSections;
		std::uint32_t m_numComponents;
		std::uint32_t m_refPoolDescOffset;
		std::uint32_t m_refPoolDataOffset;
		std::uint32_t m_namePoolDescOffset;
		std::uint32_t m_namePoolDataOffset;
		std::uint32_t m_chunkDescOffset;
		std::uint32_t m_chunkDataOffset;

		static RedPackageHeader FromCursor(FileCursor& aCursor)
        {
            RedPackageHeader header{};

			header.m_version = static_cast<std::byte>(aCursor.readByte());
            header.m_unk1 = static_cast<std::byte>(aCursor.readByte());

            header.m_numSections = aCursor.readUShort();
            header.m_numComponents = aCursor.readUInt();

            if (header.m_numSections == 7)
            {
                header.m_refPoolDescOffset = aCursor.readUInt();
                header.m_refPoolDataOffset = aCursor.readUInt();
            }

            header.m_namePoolDescOffset = aCursor.readUInt();
            header.m_namePoolDataOffset = aCursor.readUInt();

            header.m_chunkDescOffset = aCursor.readUInt();
            header.m_chunkDataOffset = aCursor.readUInt();

			return header;
		}
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

		static RedPackageImportHeader FromCursor(FileCursor& cursor) {
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

		static RedPackageNameHeader FromCursor(FileCursor& cursor) {
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

		static RedPackageChunkHeader FromCursor(FileCursor& cursor) {
			auto ret = RedPackageChunkHeader{};

			ret.typeId = cursor.readUInt();
			ret.offset = cursor.readUInt();

			return ret;
		}
	};

	struct RedChunk {
		Red::CName m_typeName;
        redRTTI::RTTIValue m_redClass;

		// I do not believe anything non-serializable can be inside scriptablesystem :P
		Red::ISerializable* m_instance;

		bool m_isUsed; // For handles, maybe, sometime later...

		// Clean the pointer up
		~RedChunk()
        {
            if (!m_instance)
            {
                return;
			}

			if (m_instance->CanBeDestructed())
            {
                m_instance->GetType()->Destruct(m_instance);
			}

			m_instance->GetAllocator()->Free(m_instance);
		}
	};

	class ScriptableSystemsContainerNode : public NodeDataInterface {
	public:
		static constexpr std::wstring_view nodeName = L"ScriptableSystemsContainer";
        static constexpr auto m_enableImports = false;
        static constexpr auto m_dumpEnumSizes = false;

		RedPackageHeader m_header;
		std::vector<Red::CRUID> m_rootCruids;
		std::vector<Red::CName> m_names;
        std::vector<RedImport> m_imports;
        std::vector<RedPackageChunkHeader> m_chunkHeaders;
		std::vector<RedChunk> m_chunks;

	private:
		RedImport ReadImport(FileCursor& cursor, RedPackageImportHeader importHeader) {
			RedImport ret{};

			ret.importFlags = importHeader.isSync() ? RedImport::ImportFlags::Obligatory : RedImport::ImportFlags::Default;
			ret.importDepotPath = cursor.readString(importHeader.size());

			return ret;
		}

	public:
		virtual void ReadData(FileCursor& cursor, NodeEntry& node) {
			const auto dataSize = cursor.readInt();
            if (dataSize == 0)
            {
                return;
			}

			{
				// Test of less allocation-happy sub-FileCursor creation
                auto subCursor = cursor.CreateSubCursor(dataSize);

				m_header = RedPackageHeader::FromCursor(subCursor);

				const auto numCruids = subCursor.readUInt();

				for (auto i = 0u; i < numCruids; i++) {
                    m_rootCruids.push_back(subCursor.readCruid());
				}

				const auto baseOffset = subCursor.offset;

				if constexpr (m_enableImports)
                {
                    subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + m_header.m_refPoolDescOffset);

                    const auto refDesc = subCursor.ReadMultipleClasses<RedPackageImportHeader>(
                        (m_header.m_refPoolDataOffset - m_header.m_refPoolDescOffset) / sizeof(RedPackageImportHeader));

                    for (auto ref : refDesc)
                    {
                        subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + ref.offset());
                        m_imports.push_back(ReadImport(subCursor, ref));
                    }
				}

				subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + m_header.m_namePoolDescOffset);
                //const auto nameDesc = subCursor.ReadMultipleClasses<RedPackageNameHeader>(
                //    (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) / sizeof(RedPackageNameHeader));

				for (auto name : subCursor.ReadMultipleClasses<RedPackageNameHeader>(
                         (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) /
                         sizeof(RedPackageNameHeader)))
                {
                    subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + name.offset());
                    m_names.push_back(subCursor.ReadCName());
				}

				subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + m_header.m_chunkDescOffset);

				m_chunkHeaders = subCursor.ReadMultipleClasses<RedPackageChunkHeader>(
                    (m_header.m_chunkDataOffset - m_header.m_chunkDescOffset) / sizeof(RedPackageChunkHeader));

                scriptable::native::ScriptableReader nativeReader{&m_names};

				m_chunks.resize(m_chunkHeaders.size());

				for (auto i = 0u; i < m_chunkHeaders.size(); i++)
                {
                    auto header = m_chunkHeaders.at(i);
                    auto& chunk = m_chunks.at(i);

					chunk.m_typeName = m_names.at(header.typeId);

					auto chunkType = PluginContext::m_rtti->GetType(chunk.m_typeName);

					// We don't know the chunk's type (or the type is wacky), blame mods
                    if (!chunkType || chunkType->GetType() != Red::ERTTIType::Class)
                    {
                        PluginContext::Error(std::format("ScriptableSystemParser: Tried to load chunk {} with no RTTI "
                                                         "representation, skipping to next chunk...",
                                                         chunk.m_typeName.ToString()));
                        continue;
                    }

					subCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + header.offset);

					auto instance = static_cast<Red::CClass*>(chunkType)->CreateInstance();

					nativeReader.ReadClass(subCursor, instance, chunkType);

					chunk.m_instance = reinterpret_cast<Red::ISerializable*>(instance);
				}
				
				// Test what enum sizes actually show up during scriptable reads
                if constexpr (m_dumpEnumSizes)
                {
                    for (auto size : scriptable::native::ScriptableReader::s_enumSizes)
                    {
                        PluginContext::Spew(std::format("Size {}", static_cast<int>(size)));
					}

					std::unordered_set<std::uint8_t> globalEnumSizes{};

					PluginContext::m_rtti->types.ForEach([&globalEnumSizes](Red::CName aName, Red::CBaseRTTIType* aType)
                        {
                            if (aType->GetType() == Red::ERTTIType::Enum)
                            {
                                globalEnumSizes.insert(static_cast<Red::CEnum*>(aType)->actualSize);
							}
                        });

					PluginContext::Spew("Enum sizes across all RTTI");

					for (auto size : globalEnumSizes)
                    {
                        PluginContext::Spew(std::format("Size {}", static_cast<int>(size)));
                    }
				}
			}
		}

		RedChunk& LookupChunk(std::string_view aChunkType) {
            auto chunkIter =
                std::find_if(m_chunks.begin(), m_chunks.end(),
                             [aChunkType](const RedChunk& aChunk) {
				return aChunk.m_typeName == aChunkType.data();
			});

			if (chunkIter == m_chunks.end())
            {
                throw std::runtime_error{std::format("Failed to find chunk {}", aChunkType)};
			}

			return *chunkIter;
		}
	};
}