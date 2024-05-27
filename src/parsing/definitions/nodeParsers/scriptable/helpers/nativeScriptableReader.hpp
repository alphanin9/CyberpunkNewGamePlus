#pragma once
#include <format>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../../../../../context/context.hpp"
#include "../../rttiHelpers/rttiValueReaderNative.hpp"

namespace scriptable::native
{
struct RedPackageFieldHeader
{
    std::uint16_t m_nameId;
    std::uint16_t m_typeId;
    std::uint32_t m_offset;

    inline static RedPackageFieldHeader FromCursor(FileCursor& cursor)
    {
        auto ret = RedPackageFieldHeader{};

        ret.m_nameId = cursor.readUShort();
        ret.m_typeId = cursor.readUShort();
        ret.m_offset = cursor.readUInt();

        return ret;
    }
};

struct OptimizedFieldHeader
{
    Red::CName m_name;
    Red::CName m_type;
    std::uint32_t m_offset;

    inline static OptimizedFieldHeader Make(RedPackageFieldHeader aHeader, const std::vector<Red::CName>* aNames)
    {
        OptimizedFieldHeader ret{};
        ret.m_name = aNames->at(aHeader.m_nameId);
        ret.m_type = aNames->at(aHeader.m_typeId);
        ret.m_offset = aHeader.m_offset;

        return ret;
    }
};

class ScriptableReader : redRTTI::native::NativeReader
{
    std::vector<Red::CName>* m_names;
    static constexpr auto m_reportNonCriticalErrors = false;

    Red::CName ReadCNameInternal(FileCursor& aCursor)
    {
        const auto index = aCursor.readUShort();

        if (index >= m_names->size())
        {
             return {};
        }

        return m_names->at(index);
    }

    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) final
    {
        *reinterpret_cast<Red::CName*>(aOut) = ReadCNameInternal(aCursor);
    }

    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) final
    {
        const auto size = aCursor.readUShort();
        const auto name = aCursor.readString(size);

        constexpr auto shouldDumpNodeRefs = false;

        if constexpr (shouldDumpNodeRefs)
        {
            PluginContext::Spew(std::format("NodeRef {}", name));
        }

        *reinterpret_cast<Red::NodeRef*>(aOut) = Red::NodeRef{name.c_str()};
    }

    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) final
    {
        auto enumValueName = ReadCNameInternal(aCursor);
        auto enumType = static_cast<Red::CEnum*>(aPropType);
        
        auto wasFound = false;
        auto enumValue = enumType->valueList.Back();
        
        for (auto i = 0; i < enumType->hashList.size; i++)
        {
            if (enumType->hashList[i] == enumValueName)
            {
                enumValue = enumType->valueList[i];
                wasFound = true;
                break;
            }
        }
        
        // Didn't find, try aliases...

        if (!wasFound)
        {
            for (auto i = 0; i < enumType->aliasList.size; i++)
            {
                if (enumType->aliasList[i] == enumValueName)
                {
                    enumValue = enumType->aliasValueList[i];
                    break;
                }
            }
        }

        s_enumSizes.insert(enumType->actualSize);
        
        // Scriptable systems only seem to use actualSize=4

        switch (enumType->actualSize)
        {
        case 1u:
            *reinterpret_cast<std::int8_t*>(aOut) = static_cast<std::int8_t>(enumValue);
            break;
        case 2u:
            *reinterpret_cast<std::int16_t*>(aOut) = static_cast<std::int16_t>(enumValue);
            break;
        case 4u:
            *reinterpret_cast<std::int32_t*>(aOut) = static_cast<std::int32_t>(enumValue);
            break;
        case 8u:
            *reinterpret_cast<std::int64_t*>(aOut) = enumValue;
            break;
        default: // What the fuck?
            break;
        }
    }

    virtual void ReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType)
    {
        // Not necessary in here, skip.
        aCursor.readInt();
    }

    virtual void ReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType)
    {
        // Again, not necessary - skip it.
        aCursor.readInt();
    }

public:
    inline static std::unordered_set<std::uint8_t> s_enumSizes;
    inline ScriptableReader(std::vector<Red::CName>* aNames)
        : m_names(aNames)
    {
    
    }

    // This puts the reader into an invalid state by default, as we don't know any names
    inline ScriptableReader() = default;

    virtual ~ScriptableReader()
    {
        
    }

    inline std::vector<Red::CName>* GetNames()
    {
        return m_names;
    }

    inline void SetNames(std::vector<Red::CName>* aNames)
    {
        m_names = aNames;
    }

    inline virtual void ReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) final
    {
        auto classType = static_cast<Red::CClass*>(aType);

        const auto baseOffset = aCursor.offset;
        const auto fieldCount = aCursor.readUShort();

        for (auto desc : aCursor.ReadMultipleClasses<RedPackageFieldHeader>(fieldCount))
        {
            const auto propName = m_names->at(desc.m_nameId);
            const auto propData = classType->GetProperty(propName);

            // No prop...
            if (!propData)
            {
                if constexpr (m_reportNonCriticalErrors)
                {
                    PluginContext::Error(std::format("NativeScriptableReader::ReadClass, couldn't find {}.{}",
                                                     classType->GetName().ToString(), propName.ToString()));
                }
                
                continue;
            }

            if (propData->type->GetName() != m_names->at(desc.m_typeId))
            {
                if constexpr (m_reportNonCriticalErrors)
                {
                    PluginContext::Error(
                        std::format("NativeScriptableReader::ReadClass, class {}, property type mismatch - {} != {}",
                                    classType->GetName().ToString(), propData->type->GetName().ToString(),
                                    m_names->at(desc.m_typeId).ToString()));
                }
                continue;
            }

            aCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + desc.m_offset);

            auto propPtr = propData->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(aOut);

            try
            {
                ReadValue(aCursor, propPtr, propData->type);
            }
            catch (const std::exception& e)
            {
                PluginContext::Error(std::format("NativeScriptableReader::ReadClass exception, {}", e.what()));
            }
        }
    }
};
}