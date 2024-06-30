#include "../../../context/context.hpp"
#include "packageReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/data/StatType.hpp>

#include <unordered_map>

// Currently unused, is meant to replace nativeScriptableReader...
namespace package
{
bool Package::ResolveEnumValue(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aRet) noexcept
{
    static EnumCache s_cache{};

    if (s_cache.Resolve(aEnum, aName, aRet))
    {
        return true;
    }

    aRet = aEnum->valueList.Back();

    for (auto i = 0; i < aEnum->hashList.size; i++)
    {
        if (aEnum->hashList[i] == aName)
        {
            aRet = aEnum->valueList[i];
            return true;
        }
    }

    for (auto i = 0; i < aEnum->aliasList.size; i++)
    {
        if (aEnum->aliasList[i] == aName)
        {
            aRet = aEnum->aliasValueList[i];
            return true;
        }
    }

    return false;
}

const char* Package::GetEnumString(Red::CEnum* aEnum, std::int64_t aValue) noexcept
{
    for (auto i = 0u; i < aEnum->valueList.size; i++)
    {
        if (aEnum->valueList[i] == aValue)
        {
            return aEnum->hashList[i].ToString();
        }
    }

    for (auto i = 0u; i < aEnum->aliasValueList.size; i++)
    {
        if (aEnum->aliasValueList[i] == aValue)
        {
            return aEnum->aliasList[i].ToString();
        }
    }

    return "Invalid";
}

bool EnumCache::Resolve(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aValue) noexcept
{
    static const auto statTypes = Red::GetEnum<Red::game::data::StatType>();

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

EnumCache::EnumCache() noexcept
{
    static const auto statTypes = Red::GetEnum<Red::game::data::StatType>();

    for (std::size_t i = 0u; i < statTypes->hashList.size; i++)
    {
        m_map.insert_or_assign(statTypes->hashList[i], statTypes->valueList[i]);
    }

    for (std::size_t i = 0u; i < statTypes->aliasList.size; i++)
    {
        m_map.insert_or_assign(statTypes->aliasList[i], statTypes->aliasValueList[i]);
    }
}

Red::CName Package::ReadCNameInternal(FileCursor& aCursor) noexcept
{
    const auto index = aCursor.readUShort();

    if (index >= m_names.size())
    {
        return {};
    }

    return m_names[index];
}

void Package::ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept
{
    *reinterpret_cast<Red::CName*>(aOut) = ReadCNameInternal(aCursor);
}

void Package::ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept
{
    const auto size = aCursor.readUShort();
    const auto name = aCursor.readString(size);

    *reinterpret_cast<Red::NodeRef*>(aOut) = Red::NodeRef{name.c_str()};
}

void Package::ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
{
    auto enumValueName = ReadCNameInternal(aCursor);
    auto enumType = static_cast<Red::CEnum*>(aPropType);

    // Note: this is actually pretty slow...
    std::int64_t enumValue{};
    ResolveEnumValue(enumType, enumValueName, enumValue);

    // Scriptable systems only seem to use actualSize=4
    if (enumType->actualSize == 0)
    {
        PluginContext::Error("Package::ReadEnum, enumType->actualSize == 0");
        return;
    }

    // Should be fine, given endianness and all that sort
    // Though maybe less performant?
    memcpy(aOut, &enumValue, enumType->actualSize);
}

bool Package::TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
{
    const auto chunkId = aCursor.readInt();

    auto handlePtr = reinterpret_cast<Red::Handle<Red::ISerializable>*>(aOut);

    auto chunkHandle = ReadChunkById(chunkId);

    handlePtr->Swap(chunkHandle);

    return true;
}

bool Package::TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
{
    const auto chunkId = aCursor.readInt();
    // Don't bother with whandles, they're not used anywhere important I don't think

    return true;
}

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

bool Package::TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept
{
    auto classType = static_cast<Red::CClass*>(aType);

    const auto baseOffset = aCursor.offset;
    const auto fieldCount = aCursor.readUShort();

    for (auto desc : aCursor.ReadSpan<PackageFieldDescriptor>(fieldCount))
    {
        const auto propData = classType->GetProperty(m_names.at(desc.m_nameId));

        // No prop...
        if (!propData)
        {
            continue;
        }

        const auto propTypeExpected = PluginContext::m_rtti->GetType(m_names.at(desc.m_typeId));

        if (propData->type != propTypeExpected)
        {
            auto isCompatible = false;

            // NOTE: we don't resolve wrefs, so we don't care about type mismatches there...
            if (propTypeExpected && propData->type->GetType() == Red::ERTTIType::Handle)
            {
                auto asHandle = static_cast<Red::CRTTIHandleType*>(propData->type);
                auto asHandleExpected = static_cast<Red::CRTTIHandleType*>(propTypeExpected);

                isCompatible =
                    static_cast<Red::CClass*>(asHandleExpected->GetInnerType())->IsA(asHandle->GetInnerType());
            }

            if (!isCompatible)
            {
                continue;
            }
        }

        aCursor.seekTo(baseOffset + desc.m_offset);

        auto propPtr = propData->GetValuePtr<void>(aOut);

        if (!TryReadValue(aCursor, propPtr, propTypeExpected))
        {
            PluginContext::Error(std::format("NativeScriptableReader::TryReadClass, failed to read prop {}::{}",
                                             classType->GetName().ToString(), propData->name.ToString()));
        }
    }

    return true;
}

Red::Handle<Red::ISerializable> Package::ReadChunkById(std::size_t aId) noexcept
{
    // Note: maybe multi-thread this?
    auto header = m_chunkHeaders[aId];

    m_cursor.seekTo(m_baseOffset + header.offset);

    auto handle = Red::MakeScriptedHandle(m_names.at(header.typeID));
    
    if (!TryReadClass(m_cursor, handle.GetPtr(), handle->GetType()))
    {
        PluginContext::Error(std::format("Package::ReadChunkById failed reading chunk {}!", aId));
    }

    return handle;
}

bool Package::ReadPackage() noexcept
{
    if (m_isRead)
    {
        return false;
    }

    m_header = PackageHeader(m_cursor);
    
    // None of our usecases need CRUIDs, but we need to read them regardless...
    if (m_readCruids)
    {
        const auto numCruids = m_cursor.readUInt();
        m_cursor.ReadSpan<Red::CRUID>(numCruids);
    }

    m_baseOffset = m_cursor.offset;

    m_cursor.seekTo(m_baseOffset + m_header.m_namePoolDescOffset);

    const auto nameSpan = m_cursor.ReadSpan<Red::PackageHeader::Name>(
        (m_header.m_namePoolDataOffset - m_header.m_namePoolDescOffset) / sizeof(Red::PackageHeader::Name));

    m_names.reserve(nameSpan.size());

    for (auto i : nameSpan)
    {
        m_cursor.seekTo(m_baseOffset + i.offset);
        m_names.push_back(m_cursor.ReadCName());
    }

    m_cursor.seekTo(m_baseOffset + m_header.m_chunkDescOffset);
    m_chunkHeaders = m_cursor.ReadSpan<Red::PackageHeader::Chunk>(
        (m_header.m_chunkDataOffset - m_header.m_chunkDescOffset) / sizeof(Red::PackageHeader::Chunk));

    // Do not go any further, any reader logic happens on access...
    return true;
}

Red::Handle<Red::ISerializable>* Package::GetChunkByTypeName(Red::CName aType) noexcept
{
    std::size_t chunkIndex = std::numeric_limits<std::size_t>::max();

    for (std::size_t i = 0; i < m_chunkHeaders.size(); i++)
    {
        auto header = m_chunkHeaders[i];

        if (m_names.at(header.typeID) == aType)
        {
            chunkIndex = i;
            break;
        }
    }

    if (chunkIndex == std::numeric_limits<std::size_t>::max())
    {
        return nullptr;
    }

    auto mapIter = m_objects.find(chunkIndex);

    if (m_objects.contains(chunkIndex))
    {
        return &m_objects.at(chunkIndex);
    }

    auto objType = PluginContext::m_rtti->GetType(aType);

    if (!objType)
    {
        PluginContext::Spew(std::format("Type {} does not have a corresponding RTTI entry!", aType.ToString()));
        return nullptr;
    }

    // How to MT this (potentially? as long as everything doesn't get turbo-fucked by race conditions ETC)? Make every TryReadValue call into a job closure?
    // Can same prop/chunk be read several times?
    // Red::JobQueue readerQueue{};

    auto objHandle = ReadChunkById(chunkIndex);

    m_objects.insert(std::make_pair(chunkIndex, std::move(objHandle)));

    return &m_objects.at(chunkIndex);
}

void Package::Init(FileCursor&& aCursor) noexcept
{
    m_cursor = aCursor;
    m_isRead = false;
    m_readCruids = false;
    m_baseOffset = 0;
}
}