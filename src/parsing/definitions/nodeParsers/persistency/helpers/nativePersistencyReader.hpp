#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../../rttiHelpers/rttiValueReaderNative.hpp"
#include <format>

namespace persistency::native
{
class NativePersistencyReader : public redRTTI::native::NativeReader
{
protected:
    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) final
    {
        *reinterpret_cast<Red::CName*>(aOut) = aCursor.ReadCNameHash();
    }

    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) final
    {
        *reinterpret_cast<Red::NodeRef*>(aOut) = aCursor.readUInt64();
    }

    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) final
    {
        auto enumType = static_cast<RED4ext::CEnum*>(aType);
        std::int64_t enumIndexer{};

        switch (enumType->actualSize)
        {
        case 1u:
            enumIndexer = aCursor.readByte();
            if (enumIndexer < enumType->valueList.size)
            {
                *reinterpret_cast<std::uint8_t*>(aOut) = static_cast<std::uint8_t>(enumIndexer);    
            }
            else
            {
                *reinterpret_cast<std::uint8_t*>(aOut) = static_cast<std::uint8_t>(enumType->valueList.Back());
            }
            break;
        case 2u:
            enumIndexer = aCursor.readShort();
            if (enumIndexer < enumType->valueList.size)
            {
                *reinterpret_cast<std::uint16_t*>(aOut) = static_cast<std::uint16_t>(enumIndexer);
            }
            else
            {
                *reinterpret_cast<std::uint16_t*>(aOut) = static_cast<std::uint16_t>(enumType->valueList.Back());
            }
            break;
        case 4u:
            enumIndexer = aCursor.readInt();
            if (enumIndexer < enumType->valueList.size)
            {
                *reinterpret_cast<std::uint32_t*>(aOut) = static_cast<std::uint32_t>(enumIndexer);
            }
            else
            {
                *reinterpret_cast<std::uint32_t*>(aOut) = static_cast<std::uint32_t>(enumType->valueList.Back());
            }
            break;
        case 8u:
            enumIndexer = aCursor.readInt64();
            if (enumIndexer < enumType->valueList.size)
            {
                *reinterpret_cast<std::uint64_t*>(aOut) = static_cast<std::uint64_t>(enumIndexer);
            }
            else
            {
                *reinterpret_cast<std::uint64_t*>(aOut) = static_cast<std::uint64_t>(enumType->valueList.Back());
            }
            break;
        default:
            throw std::runtime_error{
                std::format("Enum {} has size {}", enumType->GetName().ToString(), enumType->actualSize)};
            break;
        }
    }

    virtual void ReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) final
    {
        const auto handleType = static_cast<Red::CRTTIHandleType*>(aType);
        const auto innerType = handleType->GetInnerType();

        auto instance = innerType->GetAllocator()->AllocAligned(innerType->GetSize(), innerType->GetAlignment()).memory;
        innerType->Construct(instance);

        ReadValue(aCursor, instance, innerType);

        // Deallocate it immediately afterwards
        // This actually crashes
        // Not a problem for our usecase :P

        innerType->Destruct(instance);
        innerType->GetAllocator()->Free(instance);

        //using HandleType = Red::Handle<Red::ISerializable>;

        //*reinterpret_cast<HandleType*>(aOut) = HandleType{reinterpret_cast<Red::ISerializable*>(instance)};
    }

    virtual void ReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) final
    {
        // Skip this, currently we have no need for wrefs
        aCursor.readInt();
    }
public:
    virtual void ReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aClass) final
    {
        auto classType = static_cast<Red::CClass*>(aClass);
        try
        {
            while (aCursor.getRemaining())
            {
                auto propName = aCursor.ReadCNameHash();

                if (propName.IsNone())
                {
                    break;
                }

                auto propTypeStr = aCursor.ReadCNameHash();
                auto propInfo = classType->GetProperty(propName);

                if (!propInfo)
                {
                    throw std::runtime_error{std::format("Prop {} does not exist in class {}", propName.ToString(),
                                                         aClass->GetName().ToString())};
                }

                if (propTypeStr != propInfo->type->GetName())
                {
                    throw std::runtime_error{std::format("Prop type mismatch, {} != {} in class {} for prop {}",
                                                         propTypeStr.ToString(), propInfo->type->GetName().ToString(),
                                                         aClass->GetName().ToString(), propInfo->name.ToString())};
                }

                auto valuePtr = propInfo->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(aOut);

                if (!valuePtr)
                {
                    throw std::runtime_error{std::format("Failed to get value pointer for {}.{}",
                                                         classType->GetName().ToString(), propInfo->name.ToString())};
                }

                ReadValue(aCursor, valuePtr, propInfo->type);
            }
        }
        catch (const std::exception& e)
        {
            PluginContext::Error(std::format("NativePersistencyReader::ReadClass, {}", e.what()));
        }
    }
};
} // namespace persistency::native