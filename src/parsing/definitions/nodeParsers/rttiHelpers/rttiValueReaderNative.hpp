#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../../../../context/context.hpp"
#include "../../../cursorDef.hpp"

namespace redRTTI::native
{
// TODO: Finish noexcept variants for this
// TODO: Make this not use virtual methods, they are inefficient
class NativeReader
{
protected:
    virtual void ReadTDBID(FileCursor& aCursor, Red::ScriptInstance aOut)
    {
        *reinterpret_cast<Red::TweakDBID*>(aOut) = aCursor.readTdbId();
    }

    // Those are a little more class-defined
    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept
    {
    
    }

    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept
    {
    
    }

    // These need a bit of info about the inner RTTI type...
    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
    
    }

    bool TryReadArray(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
        const auto arraySize = aCursor.readInt();
        const auto arrayType = static_cast<Red::CRTTIBaseArrayType*>(aPropType); // Oops, had ArrayType even on static arrs...
        const auto innerType = arrayType->GetInnerType();

        arrayType->Resize(aOut, arraySize);

        for (auto i = 0; i < arraySize; i++)
        {
            // Array elements are constructed on resize, we don't need to create a new instance
            auto elem = arrayType->GetElement(aOut, i);
            if (!TryReadValue(aCursor, elem, innerType))
            {
                return false;
            }
        }

        return true;
    }

    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
        return false;
    }

    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut,
                                   Red::CBaseRTTIType* aPropType) noexcept
    {
        return false;
    }

    bool TryReadValue(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
        const auto rttiType = aPropType->GetType();
        const auto typeName = aPropType->GetName();

        using Red::ERTTIType;

        switch (rttiType)
        {
        case ERTTIType::Name:
            if (typeName == "CName")
            {
                ReadCName(aCursor, aOut);
                return true;
            }
            break;
        case ERTTIType::Fundamental:
            aCursor.CopyTo(aOut, aPropType->GetSize());
            return true;
        case ERTTIType::Class:
            return TryReadClass(aCursor, aOut, aPropType);
        case ERTTIType::Array:
        case ERTTIType::StaticArray:
        case ERTTIType::NativeArray:
            return TryReadArray(aCursor, aOut, aPropType);
        case ERTTIType::Simple:
            if (typeName == RED4ext::CName{"TweakDBID"})
            {
                ReadTDBID(aCursor, aOut);
                return true;
            }
            else if (typeName == RED4ext::CName{"NodeRef"})
            {
                ReadNodeRef(aCursor, aOut);
                return true;
            }
            break;
        case ERTTIType::Enum:
            ReadEnum(aCursor, aOut, aPropType);
            return true;
        case ERTTIType::Handle:
            return TryReadHandle(aCursor, aOut, aPropType);
        case ERTTIType::WeakHandle:
            return TryReadWeakHandle(aCursor, aOut, aPropType);
        }

        PluginContext::Error(std::format("NativeReader::TryReadValue, type {} cannot be loaded!", typeName.ToString()));
        return false;
    }

public:
    virtual ~NativeReader()
    {
    }
    virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aClass) noexcept
    {
        return false;
    }
};
} // namespace redRTTI::native