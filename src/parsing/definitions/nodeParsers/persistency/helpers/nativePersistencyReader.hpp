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

        if (enumType->actualSize == 0)
        {
            PluginContext::Error("NativePersistencyReader::ReadEnum, enumType->actualSize == 0");
        }

        aCursor.CopyTo(aOut, enumType->actualSize);
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

    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept final
    {
        const auto handleType = static_cast<Red::CRTTIHandleType*>(aType);
        auto innerType = handleType->GetInnerType();

        if (innerType->GetType() != Red::ERTTIType::Class)
        {
            PluginContext::Error(std::format("Tried to read handle of a non-class type! Typename: {}", aType->GetName().ToString()));
            return false;
        }

        auto instance = static_cast<Red::CClass*>(innerType)->CreateInstance();
        auto handle = Red::Handle<Red::ISerializable>(reinterpret_cast<Red::ISerializable*>(instance));

        if (!TryReadValue(aCursor, handle.GetPtr(), innerType))
        {
            return false;
        }

        using HandleType = Red::Handle<Red::ISerializable>;

        reinterpret_cast<HandleType*>(aOut)->Swap(handle);

        return true;
    }

    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut,
                                   Red::CBaseRTTIType* aType) noexcept final
    {
        aCursor.readInt();
        return true;
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

    virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aClass) noexcept final
    {
        auto classType = static_cast<Red::CClass*>(aClass);

        while (aCursor.getRemaining())
        {
            auto propName = aCursor.ReadCNameHash();

            if (propName.IsNone())
            {
                break;
            }

            auto propTypeStr = aCursor.ReadCNameHash();
            auto prop = classType->GetProperty(propName);

            if (!prop)
            {
                PluginContext::Error(std::format("Prop {}::{} does not exist!", classType->name.ToString(), propName.ToString()));
                return false;
            }

            if (propTypeStr != prop->type->GetName())
            {
                PluginContext::Error(
                    std::format("Prop {}::{} type mismatch, {} != {}!", classType->name.ToString(), propName.ToString(), prop->type->GetName().ToString(), propTypeStr.ToString()));
                return false;
            }

            auto valuePtr = prop->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(aOut);

            if (!TryReadValue(aCursor, valuePtr, prop->type))
            {
                PluginContext::Error(
                    std::format("Failed to read {}::{}", classType->name.ToString(), propName.ToString()));
                return false;
            }
        }

        return true;
    }
};
} // namespace persistency::native