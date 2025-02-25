#pragma once
#include <Parsing/CursorDef.hpp>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Package.hpp>
#include <RedLib.hpp>
#include <Parsing/Definitions/RTTIHelpers/RTTIValueReaderNative.hpp>

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

class EnumCache
{
private:
    // NOTE: we only store the biggest used enum - gamedataStatType, looking up through it causes a bunch of perf loss :(
    // No point wasting map lookups on smth small, we have cache for that - don't we?
    // Plus, this should be thread-safe, I think...
    using EnumValueMap = std::unordered_map<Red::CName, std::int64_t>;
    EnumValueMap m_map;
public:
    EnumCache() noexcept;
    bool Resolve(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aValue) noexcept;
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

    Red::CName ReadCNameInternal(FileCursor& aCursor) noexcept;
    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final;
    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final;
    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final;
    virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept final;

    bool TryReadRootClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept;

    Red::Handle<Red::ISerializable> ReadChunkById(std::size_t aId, bool aRoot) noexcept;

public:
    bool m_readCruids;
    bool m_useRootClassOptimization;

    // Cursor has copy ctor on purpose
    Package(FileCursor aCursor)
        : m_cursor(aCursor)
        , m_readCruids(false)
        , m_useRootClassOptimization(false)
        , m_isRead(false)
        , m_baseOffset(0)
    {
    }

    Package() = default;
    Package(Package&&) = default;

    // Helper func - generally useful, resolves enum value based on name...
    static bool ResolveEnumValue(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aRet) noexcept;

    // const char* is fine for this, enum value names are kept in CNamePool anyway...
    static const char* GetEnumString(Red::CEnum* aEnum, std::int64_t aValue) noexcept;

    // When default ctor is called...
    void Init(FileCursor&& aCursor) noexcept;

    bool ReadPackage() noexcept;

    // If a chunk with typename so-and-so is already read, returns it
    // If it is not, reads it, then returns it
    // Returns nullptr on failure
    Red::Handle<Red::ISerializable>* GetChunkByTypeName(Red::CName aType) noexcept;
};
}