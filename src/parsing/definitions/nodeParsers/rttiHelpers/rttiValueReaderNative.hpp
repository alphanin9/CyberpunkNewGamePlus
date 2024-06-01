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
    virtual void ReadNodeRef(FileCursor& aCursor, Red::ScriptInstance aOut) = 0;
    virtual void ReadCName(FileCursor& aCursor, Red::ScriptInstance aOut) = 0;

    // These need a bit of info about the inner RTTI type...
    virtual void ReadEnum(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) = 0;
    virtual void ReadArray(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType)
    {
        const auto arraySize = aCursor.readInt();
        const auto arrayType = static_cast<Red::CRTTIArrayType*>(aPropType);
        const auto innerType = arrayType->GetInnerType();

        arrayType->Resize(aOut, arraySize);

        for (auto i = 0; i < arraySize; i++)
        {
            // Array elements are constructed on resize, we don't need to create a new instance
            auto elem = arrayType->GetElement(aOut, i);
            ReadValue(aCursor, elem, innerType);
        }
    }
    // In theory, we could use the same array serialization for every array type (static arrays, dynarrays, fixed
    // arrays, native arrays...)
    virtual void ReadStaticArray(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType)
    {
        const auto arraySize = aCursor.readInt();
        const auto arrayType = static_cast<Red::CRTTIStaticArrayType*>(aPropType);

        const auto innerType = arrayType->GetInnerType();
        const auto innerTypeSize = innerType->GetSize();

        const auto maxLength = arrayType->GetMaxLength();

        for (auto i = 0; i < std::min(arraySize, maxLength - 1); i++)
        {
            // Should work nicer than the previous implementation
            auto elem = arrayType->GetElement(aOut, i);

            ReadValue(aCursor, elem, innerType);
        }
        // What's the difference between calling Resize() on a static array type at start and at end?
        arrayType->Resize(aOut, arraySize);
    }

    // Should work for arrs of all types
    // NVM, results in odd crashes
    bool TryReadArray(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
        const auto arraySize = aCursor.readInt();
        const auto arrayType = static_cast<Red::CRTTIArrayType*>(aPropType);
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

    virtual void ReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) = 0;
    virtual void ReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) = 0;

    virtual bool TryReadHandle(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType) noexcept
    {
        PluginContext::Error("Hit raw TryReadHandle!");
        return false;
    }

    virtual bool TryReadWeakHandle(FileCursor& aCursor, Red::ScriptInstance aOut,
                                   Red::CBaseRTTIType* aPropType) noexcept
    {
        PluginContext::Error("Hit raw TryReadWhandle!");
        return false;
    }

    virtual void ReadValue(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aPropType)
    {
        const auto rttiType = aPropType->GetType();
        const auto typeName = aPropType->GetName();

        using Red::ERTTIType;

        if (rttiType == ERTTIType::Name)
        {
            if (typeName == "CName")
            {
                return ReadCName(aCursor, aOut);
            }
        }
        else if (rttiType == ERTTIType::Fundamental)
        {
            return aCursor.CopyTo(aOut, aPropType->GetSize());
        }
        else if (rttiType == ERTTIType::Simple)
        {
            if (typeName == RED4ext::CName{"TweakDBID"})
            {
                return ReadTDBID(aCursor, aOut);
            }
            else if (typeName == RED4ext::CName{"NodeRef"})
            {
                return ReadNodeRef(aCursor, aOut);
            }
        }
        else if (rttiType == ERTTIType::Enum)
        {
            return ReadEnum(aCursor, aOut, aPropType);
        }
        else if (rttiType == ERTTIType::Array)
        {
            return ReadArray(aCursor, aOut, aPropType);
        }
        else if (rttiType == ERTTIType::StaticArray)
        {
            return ReadStaticArray(aCursor, aOut, aPropType);
        }
        else if (rttiType == ERTTIType::Class)
        {
            return ReadClass(aCursor, aOut, aPropType);
        }
        else if (rttiType == ERTTIType::Handle)
        {
            return ReadHandle(aCursor, aOut, aPropType);
        }
        else if (rttiType == ERTTIType::WeakHandle)
        {
            return ReadWeakHandle(aCursor, aOut, aPropType);
        }

        throw std::runtime_error(std::format("NativeReader::ReadValue, unknown value type {}", typeName.ToString()));
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
    virtual void ReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aClass) = 0;
    virtual bool TryReadClass(FileCursor& aCursor, Red::ScriptInstance aOut, Red::CBaseRTTIType* aClass) noexcept
    {
        return false;
    }
};
} // namespace redRTTI::native