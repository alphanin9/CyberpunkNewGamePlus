#pragma once

#include <format>
#include <fstream>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Package.hpp>
#include <RedLib.hpp>

#include "../defaultNodeData.hpp"
#include "helpers/classDefinitions/playerDevelopmentData.hpp"
#include "helpers/nativeScriptableReader.hpp"

#include "../../package/packageReader.hpp"

#include "../../../redscript_api/dataReaders/playerDevelopmentSystem/playerDevelopmentSystemReader.hpp"
#include "../../../redscript_api/dataReaders/equipmentExOutfitSystem/outfitSystemReader.hpp"

namespace save
{
struct RedPackageHeader
{
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

        header.m_version = aCursor.readValue<std::byte>();
        header.m_unk1 = aCursor.readValue<std::byte>();

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

struct RedPackageImportHeader
{
    std::uint32_t value;

    static constexpr auto sizeShift = 23;
    static constexpr auto offsetMask = (1u << sizeShift) - 1u;
    static constexpr auto sizeMask = 0xff << sizeShift;
    static constexpr auto syncShift = 31;

    std::uint32_t offset() const
    {
        return value & offsetMask;
    }

    std::uint8_t size() const
    {
        return static_cast<std::uint8_t>((value & sizeMask) >> sizeShift);
    }

    bool isSync() const
    {
        return value >> syncShift;
    }

    RedPackageImportHeader(std::uint32_t bitfield)
        : value{bitfield}
    {
    }

    static RedPackageImportHeader FromCursor(FileCursor& cursor)
    {
        return RedPackageImportHeader{cursor.readUInt()};
    }
};

struct RedPackageNameHeader
{
    std::uint32_t value;

    static constexpr auto sizeShift = 24;
    static constexpr auto offsetMask = 0x00FFFFFF;
    static constexpr auto sizeMask = 0xFF000000;

    std::uint32_t offset() const
    {
        return value & offsetMask;
    }

    std::uint8_t size() const
    {
        return static_cast<std::uint8_t>((value & sizeMask) >> sizeShift);
    }

    RedPackageNameHeader(std::uint32_t bitfield)
        : value{bitfield}
    {
    }

    static RedPackageNameHeader FromCursor(FileCursor& cursor)
    {
        return RedPackageNameHeader{cursor.readUInt()};
    }
};

RED4EXT_ASSERT_SIZE(RedPackageNameHeader, 4);
RED4EXT_ASSERT_OFFSET(RedPackageNameHeader, value, 0);

struct RedImport
{
    enum class ImportFlags
    {
        Default = 0x0,    // done / Load from same archive?
        Obligatory = 0x1, // also sync in packages / not used in cr2w currently (1.61)
        Template = 0x2,   // done / not used in cr2w currently (1.61)
        Soft = 0x4,       // done / Load from any archive?
        Embedded = 0x8,   // Load from embedded file
        Inplace = 0x10    // done / not used in cr2w currently (1.61)
    };

    std::uint64_t resourcePathHash;
    std::string importDepotPath;
    ImportFlags importFlags;
};

struct RedPackageChunkHeader
{
    std::uint32_t m_typeId;
    std::uint32_t m_offset;

    static RedPackageChunkHeader FromCursor(FileCursor& cursor)
    {
        auto ret = RedPackageChunkHeader{};

        ret.m_typeId = cursor.readUInt();
        ret.m_offset = cursor.readUInt();

        return ret;
    }
};

RED4EXT_ASSERT_SIZE(RedPackageChunkHeader, 8);
RED4EXT_ASSERT_OFFSET(RedPackageChunkHeader, m_typeId, 0);
RED4EXT_ASSERT_OFFSET(RedPackageChunkHeader, m_offset, 4);


struct RedChunk
{
    Red::CName m_typeName;
    // I do not believe anything non-serializable can be inside scriptablesystem :P
    Red::ISerializable* m_instance;
    Red::CBaseRTTIType* m_type;

    std::byte* m_chunkLocationPtr;

    // Meh, this is dumb, I could just use a handle I think?

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

// TODO: Make all this noexcept
class ScriptableSystemsContainerNode : public NodeDataInterface
{
public:
    static constexpr Red::CName m_nodeName = "ScriptableSystemsContainer";
    static constexpr auto m_enableImports = false;
    static constexpr auto m_dumpEnumSizes = false;

    // Won't implement for now, JobQueue is weird and makes me unhappy
    // There are better places for optimization anyway
    static constexpr auto m_multithreadReading = false;
    static constexpr auto m_lazyChunkReader = true;

private:
    RedPackageHeader m_header;
    std::vector<Red::CRUID> m_rootCruids;
    std::vector<Red::CName> m_names;
    std::vector<RedImport> m_imports;
    std::vector<RedChunk> m_chunks;

    // std::vector<Red::Handle<Red::ISerializable>> m_scriptableSystems; // TODO: use this
    std::span<RedPackageChunkHeader> m_chunkHeaders;

    scriptable::native::ScriptableReader m_reader;

    std::byte* m_baseAddress;
    std::ptrdiff_t m_baseOffset;

    RedImport ReadImport(FileCursor& cursor, RedPackageImportHeader importHeader)
    {
        RedImport ret{};

        ret.importFlags = importHeader.isSync() ? RedImport::ImportFlags::Obligatory : RedImport::ImportFlags::Default;
        ret.importDepotPath = cursor.readString(importHeader.size());

        return ret;
    }

public:
    virtual void ReadData(FileCursor& cursor, NodeEntry& node)
    {
        constexpr auto testNewScriptableLoader = true;

        if constexpr (testNewScriptableLoader)
        {
            const auto oldOffset = cursor.offset;

            const auto dataSize = cursor.readInt();

            package::Package package(cursor.CreateSubCursor(dataSize));

            package.m_readCruids = true;
            package.m_useRootClassOptimization = false;

            package.ReadPackage();

            auto playerDevelopmentSystem = package.GetChunkByTypeName("PlayerDevelopmentSystem");
            if (playerDevelopmentSystem)
            {
                auto result = PlayerDevelopmentSystemReader::GetData(playerDevelopmentSystem);

                PluginContext::Spew("Level: {} SC: {} Perks: {} Relic: {}", result.m_playerLevel,
                                    result.m_playerStreetCred, result.m_playerPerkPoints, result.m_playerRelicPoints);
            }
            else
            {
                PluginContext::Spew("Failed to find PlayerDevelopmentSystem!");
            }

            auto outfitSystem = package.GetChunkByTypeName("EquipmentEx.OutfitSystem");
            if (outfitSystem)
            {
                auto result = OutfitSystemReader::GetData(outfitSystem);

                for (auto& outfitSet : result.m_data)
                {
                    PluginContext::Spew("Outfit name: {}, item count: {}", outfitSet->m_name.ToString(),
                                        outfitSet->m_outfitParts.size);
                }
            }
            else
            {
                PluginContext::Spew("Failed to find EqEx system!");
            }


            cursor.seekTo(oldOffset);
        }

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

            for (auto i = 0u; i < numCruids; i++)
            {
                m_rootCruids.push_back(subCursor.readCruid());
            }

            m_baseOffset = subCursor.offset;
            m_baseAddress = subCursor.baseAddress;

            if constexpr (m_enableImports)
            {
                subCursor.seekTo(m_baseOffset + m_header.m_refPoolDescOffset);

                const auto refDesc = subCursor.ReadMultipleClasses<RedPackageImportHeader>(
                    (m_header.m_refPoolDataOffset - m_header.m_refPoolDescOffset) / sizeof(RedPackageImportHeader));

                for (auto ref : refDesc)
                {
                    subCursor.seekTo(m_baseOffset + ref.offset());
                    m_imports.push_back(ReadImport(subCursor, ref));
                }
            }

            subCursor.seekTo(m_baseOffset + m_header.m_namePoolDescOffset);
            // const auto nameDesc = subCursor.ReadMultipleClasses<RedPackageNameHeader>(
            //     (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) / sizeof(RedPackageNameHeader));

            for (auto name : subCursor.ReadSpan<RedPackageNameHeader>(
                     (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) / sizeof(RedPackageNameHeader)))
            {
                subCursor.seekTo(m_baseOffset + name.offset());
                m_names.push_back(subCursor.ReadCName());
            }

            // Prime the reader to read classes...
            m_reader.SetNames(&m_names);

            subCursor.seekTo(m_baseOffset + m_header.m_chunkDescOffset);

            m_chunkHeaders = subCursor.ReadSpan<RedPackageChunkHeader>(
                (m_header.m_chunkDataOffset - m_header.m_chunkDescOffset) / sizeof(RedPackageChunkHeader));

            
            // This is dumb
            m_chunks.resize(m_chunkHeaders.size());

            // We could potentially MT this? Chunks aren't particularly related to each other and we don't process
            // handles

            // Actually, a superior way to do this would be to only read the right chunk (PlayerDevelopmentData/EquipmentWhatever) on demand

            for (auto i = 0u; i < m_chunkHeaders.size(); i++)
            {
                auto header = m_chunkHeaders[i];
                auto& chunk = m_chunks.at(i);

                chunk.m_typeName = m_names.at(header.m_typeId);
                auto chunkType = PluginContext::m_rtti->GetType(chunk.m_typeName);

                // We don't know the chunk's type (or the type is wacky), blame mods
                // FWIW this doesn't report the chunk name properly, odd? Not adding stuff from m_names to CNamePool? Eh, not worth it I think
                if (!chunkType || chunkType->GetType() != Red::ERTTIType::Class)
                {
                    PluginContext::Error(std::format("ScriptableSystemParser: Tried to load chunk {} with no RTTI "
                                                     "representation, skipping to next chunk...",
                                                     chunk.m_typeName.ToString()));
                    continue;
                }

                chunk.m_type = chunkType;
                subCursor.seekTo(m_baseOffset + header.m_offset);
                
                chunk.m_chunkLocationPtr = subCursor.GetCurrentPtr();

                // Try to read everything ASAP!
                if constexpr (!m_lazyChunkReader)
                {
                    auto instance = static_cast<Red::CClass*>(chunkType)->CreateInstance();

                    m_reader.TryReadClass(subCursor, instance, chunkType);

                    chunk.m_instance = reinterpret_cast<Red::ISerializable*>(instance);
                }
                // Otherwise, delay until we're done and a chunk is requested
            }

            // Test what enum sizes actually show up during scriptable reads
            if constexpr (m_dumpEnumSizes)
            {
                for (auto size : scriptable::native::ScriptableReader::s_enumSizes)
                {
                    PluginContext::Spew(std::format("Size {}", static_cast<int>(size)));
                }

                std::unordered_set<std::uint8_t> globalEnumSizes{};

                PluginContext::m_rtti->types.ForEach(
                    [&globalEnumSizes](Red::CName aName, Red::CBaseRTTIType* aType)
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

    RedChunk* LookupChunk(std::string_view aChunkType)
    {
        auto chunkIter =
            std::find_if(m_chunks.begin(), m_chunks.end(),
                         [aChunkType](const RedChunk& aChunk) { return aChunk.m_typeName == aChunkType.data(); });

        if (chunkIter == m_chunks.end())
        {
            PluginContext::Error(std::format("Failed to find chunk {}", aChunkType));
            return nullptr;
        }

        // The chunk is bad...
        if (!chunkIter->m_type)
        {
            PluginContext::Error(std::format("Chunk {} has missing type", aChunkType));
            return nullptr;
        }

        if constexpr (!m_lazyChunkReader)
        {
            return &*chunkIter;
        }

        // We've already read this chunk...
        if (chunkIter->m_instance)
        {
            return &*chunkIter;
        }

        // Not sure if this pointer math checks out
        // With new stuff, the location pointer should stay valid as long as the parser stays valid
        FileCursor cursor{chunkIter->m_chunkLocationPtr};

        auto instance = static_cast<Red::CClass*>(chunkIter->m_type)->CreateInstance();

        if (!m_reader.TryReadClass(cursor, instance, chunkIter->m_type))
        {
            PluginContext::Error(
                std::format("ScriptableSystemsContainer::LookupChunk, failed to load chunk {}", aChunkType));
            return nullptr;
        }

        chunkIter->m_instance = reinterpret_cast<Red::ISerializable*>(instance);
        
        return &*chunkIter;
    }

    Red::ISerializable* LookupInstance(std::string_view aChunkType)
    {
        auto chunkPtr = LookupChunk(aChunkType);

        if (!chunkPtr)
        {
            return nullptr;
        }

        return chunkPtr->m_instance;
    }
};
} // namespace cyberpunk