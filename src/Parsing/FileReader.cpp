#include <array>
#include <cassert>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "FileReader.hpp"
#include "CursorDef.hpp"
#include "Definitions/Compression/CompressionHeader.hpp"
#include "Definitions/FileInfo.hpp"
#include "Definitions/NodeEntry.hpp"
#include "Definitions/NodeParsers/ParserHelper.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <lz4.h>

#include <Context/Context.hpp>

#include <Filesystem/SaveFS.hpp>
#include <Util/Threads.hpp>

// Copypasted from WolvenKit :(
// TODO: throw this out, replace with game save reader - this is shitcode anyway

using Red::CName;
using Red::CString;
using Red::JobQueue;
using Red::WaitForQueue;

namespace parser
{
bool Parser::ParseSavegame(const CString& aSaveName) noexcept
{
    if (!files::ReadSaveFileToBuffer(aSaveName, m_fileStream))
    {
        return false;
    }

    auto cursor = FileCursor{m_fileStream.data(), m_fileStream.size()};

    const auto magic = cursor.readUInt();

    if (magic != modsave::FILE_MAGIC)
    {
        return false;
    }

    m_header = modsave::SaveHeader::fromCursor(cursor);

    if (m_header.gameVersion < 2000)
    {
        return false;
    }

    auto infoStart = 0;

    cursor.seekTo(FileCursor::SeekTo::End, -8);

    infoStart = cursor.readInt();
    if (cursor.readUInt() != modsave::FILE_DONE)
    {
        return false;
    }

    cursor.seekTo(infoStart);

    if (cursor.readUInt() != modsave::FILE_NODE)
    {
        return false;
    }

    m_flatNodes = cursor.ReadMultipleClasses<modsave::NodeEntry>(cursor.readVlqInt32());

    DecompressFile();

    return LoadNodes();
}

void Parser::DecompressFile() noexcept
{
    auto fileCursor = FileCursor{m_fileStream.data(), m_fileStream.size()};

    const auto compressionTablePosition = fileCursor.findByteSequence("FZLC");

    fileCursor.seekTo(compressionTablePosition);

    const auto compressionHeader = compression::CompressionHeader::fromCursor(fileCursor);

    const auto tableEntriesCount = compressionHeader.maxEntries;

    const auto chunkSize = tableEntriesCount == 0x100 ? 0x00040000 : 0x00080000;

    m_decompressedDataRaw = std::make_unique_for_overwrite<std::byte[]>(
        compressionHeader.m_totalChunkSize *
        2); // We won't be going above this size, so I believe that this should be safe
    auto endIterator = m_decompressedDataRaw.get();

    // No two decompressed regions should be touching each other...
    JobQueue waiterJob{};

    for (auto& chunkInfo : compressionHeader.dataChunkInfo)
    {
        if (fileCursor.readUInt() == compression::CompressionBlockMagic)
        {
            JobQueue decompressionJob{};

            fileCursor.readInt();

            auto subCursor = fileCursor.CreateSubCursor(chunkInfo.compressedSize - 8);
            auto decompressedSize = chunkInfo.decompressedSize;

            decompressionJob.Dispatch(
                [subCursor, decompressedSize, endIterator]()
                {
                    LZ4_decompress_safe(reinterpret_cast<char*>(subCursor.GetCurrentPtr()),
                                        reinterpret_cast<char*>(endIterator), static_cast<int>(subCursor.size),
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

    // Hold on, ain't this just the offset of the first compression block? O_o...
    // Meh, keep it this way - the calc is basically free and gives us perfect result
    const auto nodeOffsetChangeAmount = static_cast<int>(compressionTablePosition + emptyByteSize);

    for (auto& node : m_flatNodes)
    {
        node.offset -= nodeOffsetChangeAmount;
    }

    // Timeout value is arbitrary
    // Note: we can just decompress chunks on demand, there is no need to do this
    WaitForQueue(waiterJob, std::chrono::seconds(15));
}

void Parser::FindChildren(modsave::NodeEntry& aNode, int aMaxNextId) noexcept
{
    if (aNode.childId > -1)
    {
        auto nextId = aNode.nextId;

        if (nextId == -1)
        {
            nextId = aMaxNextId;
        }

        for (auto i = aNode.childId; i < nextId; i++)
        {
            auto possibleChild = std::find_if(m_flatNodes.begin(), m_flatNodes.end(),
                                              [i](modsave::NodeEntry& aNode) { return aNode.id == i; });

            if (possibleChild != m_flatNodes.end())
            {
                if (possibleChild->childId > -1)
                {
                    FindChildren(*possibleChild, nextId);
                    aNode.addChild(&*possibleChild);
                }
                else
                {
                    if (!possibleChild->isChild)
                    {
                        aNode.addChild(&*possibleChild);
                    }
                }
            }
        }
    }
}

void Parser::CalculateTrueSizes(std::vector<modsave::NodeEntry*>& aNodes, int aMaxLength) noexcept
{
    for (auto i = 0ull; i < aNodes.size(); i++)
    {
        modsave::NodeEntry* currentNode = aNodes.at(i);
        modsave::NodeEntry* nextNode = nullptr;

        if ((i + 1) < aNodes.size())
        {
            nextNode = aNodes.at(i + 1);
        }

        if (currentNode->nodeChildren.size() > 0)
        {
            auto& nextChild = currentNode->nodeChildren.front();

            auto blobSize = nextChild->offset - currentNode->offset;
            currentNode->dataSize = blobSize;

            CalculateTrueSizes(currentNode->nodeChildren, aMaxLength);
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

                if (lastNodeEnd < aMaxLength)
                {
                    currentNode->trailingSize = aMaxLength - lastNodeEnd;
                }

                continue;
            }

            auto parentNode = currentNode->parent;

            auto nextToParentNodeIter = parentNode->nextNode;

            // Something WKit does due to:
            // This is the last child on the last node. The next valid offset would be the end of the data
            // Create a virtual node for this so the code below can grab the offset
            auto nextToParentNodeOffset = aMaxLength;

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

bool Parser::LoadNodes() noexcept
{
    FileCursor cursor(m_decompressedDataRaw.get(), m_decompressedDataSize);

    for (auto& node : m_flatNodes)
    {
        cursor.seekTo(node.offset);
        node.id = cursor.readInt();
    }

    for (auto& node : m_flatNodes)
    {
        if (!node.isChild)
        {
            FindChildren(node, static_cast<int>(m_flatNodes.size()));
        }
        if (node.nextId > -1)
        {
            node.nextNode = &*std::find_if(m_flatNodes.begin(), m_flatNodes.end(),
                                           [&node](modsave::NodeEntry& aNode) { return node.nextId == aNode.id; });
        }
    }

    for (auto& node : m_flatNodes)
    {
        if (!node.isChild)
        {
            m_nodeList.push_back(&node);
        }
    }

    CalculateTrueSizes(m_nodeList, static_cast<int>(m_decompressedDataSize));

    JobQueue delayQueue{};

    for (auto node : m_nodeList)
    {
        delayQueue.Wait(util::job::MakeJob(
            [cursor, node]()
            {
                auto cursorCopy = cursor;
                modsave::ParseNode(cursorCopy, *node);
            }));
    }

    WaitForQueue(delayQueue, std::chrono::milliseconds(1000));

    return true;
}

modsave::NodeEntry* Parser::LookupNode(CName aNodeName) noexcept
{
    auto node = std::find_if(m_nodeList.begin(), m_nodeList.end(),
                             [aNodeName](const modsave::NodeEntry* aNode) { return aNode->m_hash == aNodeName; });

    if (node == m_nodeList.end())
    {
        return nullptr;
    }

    return *node;
}
} // namespace parser