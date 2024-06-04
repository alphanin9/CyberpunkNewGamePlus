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
    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final
    {
        *reinterpret_cast<Red::CName*>(aOut) = aCursor.ReadCNameHash();
    }

    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept final
    {
        *reinterpret_cast<Red::NodeRef*>(aOut) = aCursor.readUInt64();
    }

    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aType) noexcept final
    {
        auto enumType = static_cast<RED4ext::CEnum*>(aType);

        if (enumType->actualSize == 0)
        {
            PluginContext::Error("NativePersistencyReader::ReadEnum, enumType->actualSize == 0");
        }

        aCursor.CopyTo(aOut, enumType->actualSize);
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
        
        auto handle = Red::MakeScriptedHandle(static_cast<Red::CClass*>(innerType));

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

            auto expectedType = PluginContext::m_rtti->GetType(propTypeStr);

            if (expectedType != prop->type)
            {
                auto isCompatible = false;

                // Handles can point to abstract stuff, thus we need to know expected type
                if (prop->type->GetType() == Red::ERTTIType::Handle)
                {
                    auto asHandleType = static_cast<Red::CRTTIHandleType*>(prop->type);
                    auto asHandleExpectedType = static_cast<Red::CRTTIHandleType*>(expectedType);

                    isCompatible = static_cast<Red::CClass*>(asHandleExpectedType->GetInnerType())
                                       ->IsA(asHandleType->GetInnerType());
                }

                if (!isCompatible)
                {
                    PluginContext::Error(std::format("Prop {}::{} type mismatch, {} != {}!", classType->name.ToString(),
                                                     propName.ToString(), prop->type->GetName().ToString(),
                                                     propTypeStr.ToString()));
                    return false;
                }
            }

            auto valuePtr = prop->GetValuePtr<std::remove_pointer_t<Red::ScriptInstance>>(aOut);

            if (!TryReadValue(aCursor, valuePtr, expectedType))
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