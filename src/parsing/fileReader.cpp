#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <print>
#include <tuple>
#include <vector>

#include "fileReader.hpp"

#include "definitions/compression/compressionHeader.hpp"
#include "definitions/fileInfo.hpp"
#include "definitions/nodeEntry.hpp"

#include "cursorDef.hpp"
#include "definitions/nodeParsers/parserHelper.hpp"

#include <RedLib.hpp>
#include <lz4.h>

#include "../context/context.hpp"

// Copypasted from WolvenKit :(
// TODO: Make this noexcept
namespace parser
{
bool Parser::ParseSavegame(std::filesystem::path aSavePath)
{
    const auto bufferSize = std::filesystem::file_size(aSavePath);

    m_fileStream = std::vector<std::byte>{bufferSize};
    {
        auto file = std::ifstream{aSavePath, std::ios_base::binary};
        file.read(reinterpret_cast<char*>(m_fileStream.data()), bufferSize);
    }

    {
        auto fileCursor = FileCursor{m_fileStream.data(), m_fileStream.size()};

        const auto magic = fileCursor.readUInt();

        if (magic != cyberpunk::FILE_MAGIC)
        {
            return false;
        }

        m_header = cyberpunk::SaveHeader::fromCursor(fileCursor);
    }

    if (m_header.gameVersion < 2000)
    {
        return false;
    }

    auto infoStart = 0;
    {
        auto reverseCursor = FileCursor{m_fileStream.data(), m_fileStream.size()};

        reverseCursor.seekTo(FileCursor::SeekTo::End, -8);

        infoStart = reverseCursor.readInt();
        if (reverseCursor.readUInt() != cyberpunk::FILE_DONE)
        {
            return false;
        }
    }

    auto fileCursor = FileCursor{m_fileStream.data(), m_fileStream.size()};
    fileCursor.seekTo(FileCursor::SeekTo::Start, infoStart);

    if (fileCursor.readUInt() != cyberpunk::FILE_NODE)
    {
        return false;
    }

    m_flatNodes = fileCursor.ReadMultipleClasses<cyberpunk::NodeEntry>(fileCursor.readVlqInt32());

    DecompressFile();

    return LoadNodes();
}

void Parser::DecompressFile()
{
    auto fileCursor = FileCursor{m_fileStream.data(), m_fileStream.size()};

    const auto compressionTablePosition = fileCursor.findByteSequence("FZLC");

    fileCursor.seekTo(FileCursor::SeekTo::Start, compressionTablePosition);

    const auto compressionHeader = compression::CompressionHeader::fromCursor(fileCursor);

    const auto tableEntriesCount = compressionHeader.maxEntries;

    const auto chunkSize = tableEntriesCount == 0x100 ? 0x00040000 : 0x00080000;

    m_decompressedDataRaw = std::make_unique_for_overwrite<std::byte[]>(compressionHeader.m_totalChunkSize * 2); // We won't be going above this size, so I believe that this should be safe
    auto endIterator = m_decompressedDataRaw.get();

    // No two decompressed regions should be touching each other...
    Red::JobQueue waiterJob{};

    for (auto& chunkInfo : compressionHeader.dataChunkInfo)
    {
        if (fileCursor.readUInt() == compression::COMPRESSION_BLOCK_MAGIC)
        {
            Red::JobQueue decompressionJob{};

            fileCursor.readInt();

            auto subCursor = fileCursor.CreateSubCursor(chunkInfo.compressedSize - 8);
            auto decompressedSize = chunkInfo.decompressedSize;

            decompressionJob.Dispatch(
                [subCursor, decompressedSize, endIterator]() {
                    LZ4_decompress_safe(reinterpret_cast<char*>(subCursor.GetCurrentPtr()),
                                        reinterpret_cast<char*>(endIterator), subCursor.size,
                                        decompressedSize);
                });

            endIterator += chunkInfo.decompressedSize;
            m_decompressedDataSize += chunkInfo.decompressedSize;

            waiterJob.Wait(decompressionJob.Capture());
        }
        else
        {
            fileCursor.offset -= 4;

            fileCursor.CopyTo(endIterator, chunkInfo.compressedSize);

            endIterator += chunkInfo.compressedSize;
            m_decompressedDataSize += chunkInfo.compressedSize;
        }
    }

    const auto chunkCountActual = (m_decompressedDataSize / chunkSize) + 1;

    // The size of the compression table
    auto emptyByteSize = sizeof(uint32_t) + sizeof(int);
    emptyByteSize += chunkCountActual * (sizeof(int) * 3);
    emptyByteSize += (tableEntriesCount - chunkCountActual) * 12;

    const auto nodeOffsetChangeAmount = compressionTablePosition + emptyByteSize;

    for (auto& node : m_flatNodes)
    {
        node.offset -= nodeOffsetChangeAmount;
    }

    // Timeout value is arbitrary
    Red::WaitForQueue(waiterJob, std::chrono::seconds(1));
}

void Parser::FindChildren(cyberpunk::NodeEntry& node, int maxNextId)
{
    if (node.childId > -1)
    {
        auto nextId = node.nextId;

        if (nextId == -1)
        {
            nextId = maxNextId;
        }

        for (auto i = node.childId; i < nextId; i++)
        {
            auto possibleChild = std::find_if(m_flatNodes.begin(), m_flatNodes.end(),
                                              [i](cyberpunk::NodeEntry& node) { return node.id == i; });

            if (possibleChild != m_flatNodes.end())
            {
                if (possibleChild->childId > -1)
                {
                    FindChildren(*possibleChild, nextId);
                    node.addChild(&*possibleChild);
                }
                else
                {
                    if (!possibleChild->isChild)
                    {
                        node.addChild(&*possibleChild);
                    }
                }
            }
        }
    }
}

void Parser::CalculateTrueSizes(std::vector<cyberpunk::NodeEntry*>& nodes, int maxLength)
{
    for (auto i = 0ull; i < nodes.size(); i++)
    {
        cyberpunk::NodeEntry* currentNode = nodes.at(i);
        cyberpunk::NodeEntry* nextNode = nullptr;

        if ((i + 1) < nodes.size())
        {
            nextNode = nodes.at(i + 1);
        }

        if (currentNode->nodeChildren.size() > 0)
        {
            auto& nextChild = currentNode->nodeChildren.front();

            auto blobSize = nextChild->offset - currentNode->offset;
            currentNode->dataSize = blobSize;

            CalculateTrueSizes(currentNode->nodeChildren, maxLength);
        }
        else
        {
            currentNode->dataSize = currentNode->size;
        }

        if (nextNode)
        {
            auto blobSize = nextNode->offset - (currentNode->offset + currentNode->size);
            currentNode->trailingSize = blobSize;
        }
        else
        {
            if (!currentNode->parent)
            {
                auto lastNodeEnd = currentNode->offset + currentNode->size;

                if (lastNodeEnd < maxLength)
                {
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

            if (nextToParentNodeIter)
            {
                nextToParentNodeOffset = nextToParentNodeIter->offset;
            }

            auto parentMax = parentNode->offset + parentNode->size;
            auto childMax = currentNode->offset + currentNode->size;

            auto blobSize = nextToParentNodeOffset - childMax;

            if (parentMax > childMax)
            {
                currentNode->trailingSize = blobSize;
            }
            else if (parentMax == childMax)
            {
                parentNode->trailingSize = blobSize;
            }
        }
    }
}

// The most reasonable course of action is keeping flatNodes allocated as long as possible, and making the actual node
// list hold ptrs to nodes in flatNodes
bool Parser::LoadNodes()
{
    FileCursor cursor(m_decompressedDataRaw.get(), m_decompressedDataSize);

    for (auto& node : m_flatNodes)
    {
        cursor.seekTo(FileCursor::SeekTo::Start, node.offset);
        node.id = cursor.readInt();
    }

    for (auto& node : m_flatNodes)
    {
        if (!node.isChild)
        {
            FindChildren(node, m_flatNodes.size());
        }
        if (node.nextId > -1)
        {
            node.nextNode = &*std::find_if(m_flatNodes.begin(), m_flatNodes.end(),
                                           [&node](cyberpunk::NodeEntry& aNode) { return node.nextId == aNode.id; });
        }
    }

    for (auto& node : m_flatNodes)
    {
        if (!node.isChild)
        {
            m_nodeList.push_back(&node);
        }
    }

    CalculateTrueSizes(m_nodeList, m_decompressedDataSize);

    for (auto& node : m_flatNodes)
    {
        if (!node.isReadByParent)
        {
            cyberpunk::ParseNode(cursor, node);

            const auto readSize = cursor.offset - node.offset;
            const auto expectedSize = node.GetExpectedSize();

            if (readSize != expectedSize)
            {
                // HACK: itemData gets really fucked by this, even on a known good implementation
                // This is kinda irrelevant actually :P
                // We don't care about saving back anyway
                if (node.m_hash != "itemData")
                {
                    PluginContext::Error(
                        std::format("Node {} expected size {} != read size {}", node.name, expectedSize, readSize));
                }
            }
        }
    }

    return true;
}

cyberpunk::NodeEntry* Parser::LookupNode(Red::CName aNodeName) noexcept
{
    auto node = std::find_if(m_nodeList.begin(), m_nodeList.end(),
                             [aNodeName](const cyberpunk::NodeEntry* aNode) { return aNode->m_hash == aNodeName; });

    if (node == m_nodeList.end())
    {
        return nullptr;
    }

    return *node;
}
} // namespace parser