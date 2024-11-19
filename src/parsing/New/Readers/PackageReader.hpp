#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Package.hpp>

#include "BaseNativeReader.hpp"

namespace parser::reader
{
struct PackageHeader
{
    using Name = Red::PackageHeader::Name;
    using Chunk = Red::PackageHeader::Chunk;

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

    PackageHeader(Cursor& aCursor)
    {
        m_version = aCursor.ReadPrimitive<std::byte>();
        m_unk1 = aCursor.ReadPrimitive<std::byte>();

        m_numSections = aCursor.ReadPrimitive<std::uint16_t>();
        m_numComponents = aCursor.ReadPrimitive<int>();

        if (m_numSections == 7)
        {
            m_refPoolDescOffset = aCursor.ReadPrimitive<std::uint32_t>();
            m_refPoolDataOffset = aCursor.ReadPrimitive<std::uint32_t>();
        }

        m_namePoolDescOffset = aCursor.ReadPrimitive<std::uint32_t>();
        m_namePoolDataOffset = aCursor.ReadPrimitive<std::uint32_t>();

        m_chunkDescOffset = aCursor.ReadPrimitive<std::uint32_t>();
        m_chunkDataOffset = aCursor.ReadPrimitive<std::uint32_t>();
    }
};

class EnumCache
{
private:
    using EnumValueMap = std::unordered_map<Red::CName, std::int64_t>;
    EnumValueMap m_map;

public:
    EnumCache() noexcept;
    bool Resolve(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aValue) noexcept;
};

// Generic package parser
// Works for StatsSystem and ScriptableSystemsContainer
class PackageReader : public BaseNativeReader
{
private:
    Red::RawBuffer m_packageBuffer{};
    PackageHeader m_header{};
    Cursor m_cursor{};

    bool m_isRead{};

    // Base offset (after header and CRUID, if applicable)
    std::ptrdiff_t m_baseOffset{};

    // Names (better to read them in advance of their use, I think)
    std::vector<Red::CName> m_names{};
    std::span<Red::PackageHeader::Chunk> m_chunkHeaders{};
    std::unordered_map<std::size_t, Red::Handle<Red::ISerializable>> m_objects{};

    Red::CName GetName(Cursor& aCursor) noexcept;
    
    void ReadCName(Red::CName& aName, Cursor& aCursor) noexcept;
    void ReadNodeRef(Red::NodeRef& aNodeRef, Cursor& aCursor) noexcept;
    void ReadEnum(Red::CEnum* aType, Red::ScriptInstance aInstance, Cursor& aCursor) noexcept;
    bool ReadHandle(Red::Handle<Red::ISerializable>& aHandle, Cursor& aCursor) noexcept;
    bool ReadWeakHandle(Red::WeakHandle<Red::ISerializable>& aHandle, Cursor& aCursor) noexcept;
    bool ReadClass(Red::CClass* aType, Red::ScriptInstance aInstance, Cursor& aCursor) noexcept;

    Red::Handle<Red::ISerializable> ReadChunkById(std::size_t aId) noexcept;

public:
    bool m_readCruids;

    PackageReader() = default;
    PackageReader(PackageReader&&) = default;

    // Helper func - resolves enum value based on name
    static void ResolveEnumValue(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aRet) noexcept;

    // We need to purposefully init with our buffer
    void Init(Red::RawBuffer&& aBuffer) noexcept;

    bool ReadPackage() noexcept;

    // If a chunk with typename so-and-so is already read, returns it
    // If it is not, reads it, then returns it
    // Returns null handle on failure
    Red::Handle<Red::ISerializable>& GetChunkByTypeName(Red::CName aType) noexcept;
};
}