#include <context/context.hpp>

#include "PackageReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/StatIDType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/StatType.hpp>

using namespace parser;
using namespace Red;

#pragma region Enum cache for common enums
bool reader::EnumCache::Resolve(CEnum* aEnum, CName aName, std::int64_t& aValue) noexcept
{
    static const auto statTypes = GetEnum<game::data::StatType>();
    static const auto statIdTypes = GetEnum<game::StatIDType>();

    if (aEnum == statIdTypes)
    {
        // game::StatIDType::EntityID
        // game::StatIDType::ItemID
        // game::StatIDType::Invalid

        constexpr CName entityId = "EntityID";
        constexpr CName itemId = "ItemID";

        switch (aName.hash)
        {
        case entityId.hash:
            aValue = static_cast<std::int64_t>(game::StatIDType::EntityID);
            return true;
        case itemId.hash:
            aValue = static_cast<std::int64_t>(game::StatIDType::ItemID);
            return true;
        default:
            aValue = static_cast<std::int64_t>(game::StatIDType::Invalid);
            return true;
        }
    }

    if (aEnum != statTypes)
    {
        return false;
    }

    if (!m_map.contains(aName))
    {
        aValue = statTypes->valueList.Back();
        return true;
    }

    aValue = m_map[aName];
    return true;
}

reader::EnumCache::EnumCache() noexcept
{
    static const auto statTypes = GetEnum<game::data::StatType>();

    for (auto i = 0u; i < statTypes->hashList.size; i++)
    {
        m_map.insert_or_assign(statTypes->hashList[i], statTypes->valueList[i]);
    }
}
#pragma endregion

#pragma region FieldDesc
struct PackageFieldDescriptor
{
    std::uint16_t m_nameId;
    std::uint16_t m_typeId;
    std::uint32_t m_offset;
};

RED4EXT_ASSERT_SIZE(PackageFieldDescriptor, 8);
RED4EXT_ASSERT_OFFSET(PackageFieldDescriptor, m_nameId, 0);
RED4EXT_ASSERT_OFFSET(PackageFieldDescriptor, m_typeId, 2);
RED4EXT_ASSERT_OFFSET(PackageFieldDescriptor, m_offset, 4);
#pragma endregion

#pragma region Initialization
void reader::PackageReader::Init(RawBuffer&& aBuffer) noexcept
{
    new (&m_packageBuffer) RawBuffer(std::move(aBuffer));

    m_cursor = Cursor(reinterpret_cast<uintptr_t>(m_packageBuffer.data), m_packageBuffer.size);
}

bool reader::PackageReader::ReadPackage() noexcept
{
    if (m_isRead)
    {
        return false;
    }

    m_header = PackageHeader(m_cursor);

    if (m_readCruids)
    {
        const auto numCruids = m_cursor.ReadPrimitive<std::uint32_t>();
        m_cursor.ReadSpan<CRUID>(numCruids);
    }

    m_baseOffset = m_cursor.m_offset;

    m_cursor.SeekTo(m_baseOffset + m_header.m_namePoolDescOffset);

    const auto nameSpan = m_cursor.ReadSpan<PackageHeader::Name>(
        (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) / sizeof(PackageHeader::Name));

    m_names.reserve(nameSpan.size());

    for (auto i : nameSpan)
    {
        m_cursor.SeekTo(m_baseOffset + i.offset);
        m_names.push_back(m_cursor.ReadUnknownLengthName());
    }

    m_cursor.SeekTo(m_baseOffset + m_header.m_chunkDescOffset);
    m_chunkHeaders = m_cursor.ReadSpan<PackageHeader::Chunk>(
        (m_header.m_chunkDataOffset - m_header.m_chunkDescOffset) / sizeof(PackageHeader::Chunk));

    // Do not go any further, any reader logic happens on access...
    return true;
}
#pragma endregion

#pragma region Enums
void reader::PackageReader::ResolveEnumValue(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aRet) noexcept
{
    static EnumCache s_cache{};

    if (s_cache.Resolve(aEnum, aName, aRet))
    {
        return;
    }

    for (auto i = 0u; i < aEnum->hashList.size; i++)
    {
        if (aEnum->hashList[i] == aName)
        {
            aRet = aEnum->valueList[i];
            return;
        }
    }

    aRet = aEnum->valueList.Back();
}

void reader::PackageReader::ReadEnum(CEnum* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    auto enumHash = GetName(aCursor);

    std::int64_t enumValue{};

    ResolveEnumValue(aType, enumHash, enumValue);

    std::copy_n(&enumValue, aType->actualSize, aInstance);
}
#pragma endregion

#pragma region CName
CName reader::PackageReader::GetName(Cursor& aCursor) noexcept
{
    return m_names[aCursor.ReadPrimitive<std::uint16_t>()];
}

void reader::PackageReader::ReadCName(CName& aName, Cursor& aCursor) noexcept
{
    aName = GetName(aCursor);
}
#pragma endregion

#pragma region NodeRef
void reader::PackageReader::ReadNodeRef(NodeRef& aNodeRef, Cursor& aCursor) noexcept
{
    auto size = aCursor.ReadPrimitive<std::uint16_t>();

    aNodeRef = aCursor.ReadKnownLengthString(size).c_str();
}
#pragma endregion

#pragma region Handles
bool reader::PackageReader::ReadHandle(Handle<ISerializable>& aHandle, Cursor& aCursor) noexcept
{
    auto chunkId = aCursor.ReadPrimitive<int>();
    const auto currentOffset = aCursor.m_offset;

    aHandle = ReadChunkById(chunkId);

    aCursor.SeekTo(currentOffset);

    return true;
}

bool reader::PackageReader::ReadWeakHandle(WeakHandle<ISerializable>& aHandle, Cursor& aCursor) noexcept
{
    // No point
    aCursor.ReadPrimitive<int>();

    return true;
}
#pragma endregion

#pragma region Class
bool reader::PackageReader::ReadClass(CClass* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    const auto baseOffset = aCursor.m_offset;
    const auto fieldCount = aCursor.ReadPrimitive<std::uint16_t>();

    for (auto desc : aCursor.ReadSpan<PackageFieldDescriptor>(fieldCount))
    {
        const auto propData = aType->GetProperty(m_names[desc.m_nameId]);

        // No prop...
        if (!propData)
        {
            continue;
        }

        // Don't use .at(), we're noexcept anyway - no difference between segfault and abort()
        const auto propTypeExpected = PluginContext::m_rtti->GetType(m_names[desc.m_typeId]);

        if (!IsCompatible(propTypeExpected, propData->type))
        {
            // This can, in theory, cause issues with array reading if incompatible val ends up the last field in
            // class...
            continue;
        }

        aCursor.SeekTo(baseOffset + desc.m_offset);

        auto propPtr = propData->GetValuePtr<void>(aInstance);

        if (!this->ReadProperty(propTypeExpected, propPtr, aCursor))
        {
            PluginContext::Error("reader::PackageReader::ReadClass, failed to read prop {}::{}",
                                 aType->GetName().ToString(), propData->name.ToString());
        }
    }

    return true;
}
#pragma endregion

#pragma region Internal chunk reader + API
Handle<ISerializable> reader::PackageReader::ReadChunkById(std::size_t aId) noexcept
{
    auto header = m_chunkHeaders[aId];

    m_cursor.SeekTo(m_baseOffset + header.offset);

    if (m_objects.contains(aId))
    {
        return m_objects[aId];
    }

    auto handle = MakeScriptedHandle(m_names[header.typeID]);
    const auto ret = ReadClass(handle->GetType(), handle.GetPtr(), m_cursor);
    
    if (!ret)
    {
        PluginContext::Error("Package::ReadChunkById failed reading chunk {}!", aId);
    }

    m_objects.insert_or_assign(aId, handle);

    return handle;
}

Handle<ISerializable>& reader::PackageReader::GetChunkByTypeName(CName aType) noexcept
{
    static Handle<ISerializable> s_null{};

    std::size_t chunkIndex = 0u;

    for (; chunkIndex < m_chunkHeaders.size(); chunkIndex++)
    {
        auto header = m_chunkHeaders[chunkIndex];

        if (m_names[header.typeID] == aType)
        {
            break;
        }
    }

    if (chunkIndex == m_chunkHeaders.size())
    {
        return s_null;
    }

    auto mapIter = m_objects.find(chunkIndex);

    if (!m_objects.contains(chunkIndex))
    {
        auto objType = PluginContext::m_rtti->GetType(aType);

        if (!objType)
        {
            PluginContext::Spew("Type {} does not have a corresponding RTTI entry!", aType.ToString());
            return s_null;
        }

        auto objHandle = ReadChunkById(chunkIndex);

        m_objects.insert(std::make_pair(chunkIndex, std::move(objHandle)));
    }

    return m_objects.at(chunkIndex);
}
#pragma endregion