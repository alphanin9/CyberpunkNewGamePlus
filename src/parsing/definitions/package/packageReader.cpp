#include "PackageReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/StatIDType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/StatType.hpp>

#include <chrono>
#include <unordered_map>

#include <Context/Context.hpp>

// TODO: throw this out, replace with game impl
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

    for (auto i = 0u; i < aEnum->hashList.size; i++)
    {
        if (aEnum->hashList[i] == aName)
        {
            aRet = aEnum->valueList[i];
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

    return "Invalid";
}

bool EnumCache::Resolve(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aValue) noexcept
{
    // NOTE: another very often hit one is StatIDType
    static const auto statTypes = Red::GetEnum<Red::game::data::StatType>();
    static const auto statIdTypes = Red::GetEnum<Red::game::StatIDType>();

    if (aEnum == statIdTypes)
    {
        // Red::game::StatIDType::EntityID
        // Red::game::StatIDType::ItemID
        // Red::game::StatIDType::Invalid

        constexpr Red::CName entityId = "EntityID";
        constexpr Red::CName itemId = "ItemID";

        switch (aName.hash)
        {
        case entityId.hash:
            aValue = static_cast<std::int64_t>(Red::game::StatIDType::EntityID);
            return true;
        case itemId.hash:
            aValue = static_cast<std::int64_t>(Red::game::StatIDType::ItemID);
            return true;
        default:
            aValue = static_cast<std::int64_t>(Red::game::StatIDType::Invalid);
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

EnumCache::EnumCache() noexcept
{
    static const auto statTypes = Red::GetEnum<Red::game::data::StatType>();

    for (auto i = 0u; i < statTypes->hashList.size; i++)
    {
        m_map.insert_or_assign(statTypes->hashList[i], statTypes->valueList[i]);
    }

    for (auto i = 0u; i < statTypes->aliasList.size; i++)
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
        return;
    }

    // Should be fine, given endianness and all that sort
    // Though maybe less performant?
    memcpy(aOut, &enumValue, enumType->actualSize);
}

bool Package::TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
{
    const auto chunkId = aCursor.readInt();
    const auto cursorOffset = aCursor.offset;

    auto handlePtr = reinterpret_cast<Red::Handle<Red::ISerializable>*>(aOut);

    auto chunkHandle = ReadChunkById(chunkId, false);

    aCursor.seekTo(cursorOffset);

    *handlePtr = chunkHandle;

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

bool Package::TryReadRootClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept
{
    auto classType = static_cast<Red::CClass*>(aType);

    const auto baseOffset = aCursor.offset;
    const auto fieldCount = aCursor.readUShort();

    // Mostly a copy of TryReadClass with the difference of using MT for property reading...

    Red::JobQueue readerQueue{};

    for (auto desc : aCursor.ReadSpan<PackageFieldDescriptor>(fieldCount))
    {
        const auto propData = classType->GetProperty(m_names[desc.m_nameId]);

        // No prop...
        if (!propData)
        {
            continue;
        }

        // Don't use .at(), we're noexcept anyway - no difference between segfault and abort()
        const auto propTypeExpected = PluginContext::m_rtti->GetType(m_names[desc.m_typeId]);

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

        Red::JobQueue reader{};

        aCursor.seekTo(baseOffset + desc.m_offset);
        FileCursor propCursor{aCursor.GetCurrentPtr()};

        auto propPtr = propData->GetValuePtr<void>(aOut);

        reader.Dispatch(
            [this, propCursor, propPtr, propTypeExpected] { 
                auto cursorCpy = propCursor;
                this->TryReadValue(cursorCpy, propPtr, propTypeExpected);
            });

        readerQueue.Wait(reader.Capture());
    }

    Red::WaitForQueue(readerQueue, std::chrono::seconds(5));

    return true;
}

bool Package::TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept
{
    // NOTE:
    // I do not believe this can be multi-threaded with any luck
    // The issue lies with arrays
    // We don't know the final size of whatever we're reading before we actually read it
    // Thus we run into unfixable (as far as I can guess) issues...

    auto classType = static_cast<Red::CClass*>(aType);

    const auto baseOffset = aCursor.offset;
    const auto fieldCount = aCursor.readUShort();

    for (auto desc : aCursor.ReadSpan<PackageFieldDescriptor>(fieldCount))
    {
        const auto propData = classType->GetProperty(m_names[desc.m_nameId]);

        // No prop...
        if (!propData)
        {
            continue;
        }

        // Don't use .at(), we're noexcept anyway - no difference between segfault and abort()
        const auto propTypeExpected = PluginContext::m_rtti->GetType(m_names[desc.m_typeId]);

        if (!Red::IsCompatible(propTypeExpected, propData->type))
        {
            // This can, in theory, cause issues with array reading if incompatible val ends up the last field in class...
            continue;
        }

        aCursor.seekTo(baseOffset + desc.m_offset);

        auto propPtr = propData->GetValuePtr<void>(aOut);

        if (!this->TryReadValue(aCursor, propPtr, propTypeExpected))
        {
            PluginContext::Error("NativeScriptableReader::TryReadClass, failed to read prop {}::{}",
                                             classType->GetName().ToString(), propData->name.ToString());
        }
    }

    return true;
}

Red::Handle<Red::ISerializable> Package::ReadChunkById(std::size_t aId, bool aRoot) noexcept
{
    // Note: maybe multi-thread this?
    auto header = m_chunkHeaders[aId];

    m_cursor.seekTo(m_baseOffset + header.offset);

    if (!m_useRootClassOptimization && m_objects.contains(aId))
    {
        return m_objects[aId];
    }

    auto handle = Red::MakeScriptedHandle(m_names[header.typeID]);
    auto ret = false;

    if (aRoot)
    {
        ret = TryReadRootClass(m_cursor, handle.GetPtr(), handle->GetType());
    }
    else
    {
        ret = TryReadClass(m_cursor, handle.GetPtr(), handle->GetType());
    }
    
    if (!ret)
    {
        PluginContext::Error("Package::ReadChunkById failed reading chunk {}!", aId);
    }

    m_objects.insert_or_assign(aId, handle);

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
    std::size_t chunkIndex = 0;

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
        PluginContext::Spew("Type {} does not have a corresponding RTTI entry!", aType.ToString());
        return nullptr;
    }

    // How to MT this (potentially? as long as everything doesn't get turbo-fucked by race conditions ETC)? Make every TryReadValue call into a job closure?
    // Can same prop/chunk be read several times?
    // NOTE: issue is very different and not really solvable
    // Root object could potentially have MT, but nothing else (otherwise arrays etc. get fucked)!
    // Red::JobQueue readerQueue{};

    auto objHandle = ReadChunkById(chunkIndex, m_useRootClassOptimization);

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