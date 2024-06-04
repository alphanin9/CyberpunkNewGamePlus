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
};

RED4EXT_ASSERT_SIZE(RedPackageFieldHeader, 8);
RED4EXT_ASSERT_OFFSET(RedPackageFieldHeader, m_nameId, 0);
RED4EXT_ASSERT_OFFSET(RedPackageFieldHeader, m_typeId, 2);
RED4EXT_ASSERT_OFFSET(RedPackageFieldHeader, m_offset, 4);

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

    bool ResolveEnumValue(Red::CEnum* aEnum, Red::CName aName, std::int64_t& aRet)
    {
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

    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final
    {
        *reinterpret_cast<Red::CName*>(aOut) = ReadCNameInternal(aCursor);
    }

    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final
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

    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final
    {
        auto enumValueName = ReadCNameInternal(aCursor);
        auto enumType = static_cast<Red::CEnum*>(aPropType);
        
        std::int64_t enumValue{};
        ResolveEnumValue(enumType, enumValueName, enumValue);

        constexpr auto shouldDumpEnumSizes = false;
        if constexpr (shouldDumpEnumSizes)
        {
            s_enumSizes.insert(enumType->actualSize);
        }
        
        // Scriptable systems only seem to use actualSize=4
        if (enumType->actualSize == 0)
        {
            PluginContext::Error("NativeScriptableReader::ReadEnum, enumType->actualSize == 0");
            return;
        }

        // Should be fine, given endianness and all that sort
        // Though maybe less performant?
        memcpy(aOut, &enumValue, enumType->actualSize);
    }

    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final
    {
        // We don't resolve handles... yet....
        // And probably never will (actually NVM, might do it sometime...)
        // Will need alterations to ScriptableReader, though :P
        aCursor.readInt();
        return true;
    }

    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept final
    {
        // Again, no resolving handles just yet
        aCursor.readInt();
        return true;
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

    inline virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept final
    {
        auto classType = static_cast<Red::CClass*>(aType);

        const auto baseOffset = aCursor.offset;
        const auto fieldCount = aCursor.readUShort();

        for (auto desc : aCursor.ReadSpan<RedPackageFieldHeader>(fieldCount))
        {
            const auto propName = m_names->at(desc.m_nameId);
            const auto propData = classType->GetProperty(propName);

            // No prop...
            if (!propData)
            {
                if constexpr (m_reportNonCriticalErrors)
                {
                    PluginContext::Error(std::format("NativeScriptableReader::TryReadClass, couldn't find {}.{}",
                                                     classType->GetName().ToString(), propName.ToString()));
                }

                continue;
            }

            const auto propTypeName = m_names->at(desc.m_typeId);
            const auto propTypeExpected = PluginContext::m_rtti->GetType(propTypeName);

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
                    if constexpr (m_reportNonCriticalErrors)
                    {
                        PluginContext::Error(std::format(
                            "NativeScriptableReader::TryReadClass, class {}, property type mismatch - {} != {}",
                            classType->GetName().ToString(), propData->type->GetName().ToString(),
                            m_names->at(desc.m_typeId).ToString()));
                    }

                    continue;
                }
            }

            aCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + desc.m_offset);

            auto propPtr = propData->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(aOut);

            if (!TryReadValue(aCursor, propPtr, propTypeExpected))
            {
                PluginContext::Error(std::format("NativeScriptableReader::TryReadClass, failed to read prop {}::{}",
                                                 classType->GetName().ToString(), propData->name.ToString()));
            }
        }

        return true;
    }
};
}