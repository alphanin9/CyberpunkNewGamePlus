#pragma once
// Later...
#include "../../cursorDef.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Package.hpp>
#include <RedLib.hpp>

#include "../rttiHelpers/rttiValueReaderNative.hpp"

#include <unordered_map>

namespace package
{
struct PackageHeader
{
    std::byte m_version{};
    std::byte m_unk1{};
    std::uint16_t m_numSections{};
    std::uint32_t m_numComponents{};
    std::uint32_t m_refPoolDescOffset{};
    std::uint32_t m_refPoolDataOffset{};
    std::uint32_t m_namePoolDescOffset{};
    std::uint32_t m_namePoolDataOffset{};
    std::uint32_t m_chunkDescOffset{};
    std::uint32_t m_chunkDataOffset{};

    PackageHeader() = default;

    PackageHeader(FileCursor& aCursor)
    {
        m_version = aCursor.readValue<std::byte>();
        m_unk1 = aCursor.readValue<std::byte>();

        m_numSections = aCursor.readUShort();
        m_numComponents = aCursor.readInt();

        if (m_numSections == 7)
        {
            m_refPoolDescOffset = aCursor.readUInt();
            m_refPoolDataOffset = aCursor.readUInt();
        }

        m_namePoolDescOffset = aCursor.readUInt();
        m_namePoolDataOffset = aCursor.readUInt();

        m_chunkDescOffset = aCursor.readUInt();
        m_chunkDataOffset = aCursor.readUInt();
    }
};

// Generic package parser for packages found in saves...
class Package : public redRTTI::native::NativeReader
{
private:
    PackageHeader m_header;
    FileCursor m_cursor;

    bool m_isRead;

    // Base offset (after header and CRUID, if applicable)
    std::ptrdiff_t m_baseOffset;

    // Names (better to read them in advance of their use, I think)
    std::vector<Red::CName> m_names;
    std::span<Red::PackageHeader::Chunk> m_chunkHeaders;
    std::unordered_map<std::size_t, Red::Handle<Red::ISerializable>> m_objects;

    // Cursor has copy ctor on purpose
    Red::CName ReadCNameInternal(FileCursor& aCursor) noexcept;
    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final;
    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final;
    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept final;

    Red::Handle<Red::ISerializable> ReadChunkById(std::size_t aId) noexcept;

public:
    bool m_readCruids;

    Package(FileCursor aCursor)
        : m_cursor(aCursor)
        , m_readCruids(false)
        , m_isRead(false)
    {
    }

    bool ReadPackage() noexcept;

    // If a chunk with typename so-and-so is already read, returns it
    // If it is not, reads it, then returns it
    // Returns nullptr on failure
    Red::Handle<Red::ISerializable>* GetChunkByTypeName(Red::CName aType) noexcept;
};
}