#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>
#include <context.hpp>

#include "../../cursorDef.hpp"

#include <algorithm>

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

        // Optimization: simple types can just be copied... (I think?)
        if (innerType->GetType() == Red::ERTTIType::Fundamental)
        {
            auto begin = arrayType->GetElement(aOut, 0);
            aCursor.CopyTo(begin, arraySize * innerType->GetSize());

            return true;
        }

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

    void ReadDataBuffer(FileCursor& aCursor, Red::ScriptInstance aOut) noexcept
    {
        // NOTE: this is necessary because, unlike SDK RawBuffer dtor, game RawBuffer dtor does not check if buf allocator is valid...
        // Since we already manage the memory of decompressed save data and we don't use classes after parser gets destructed, it should be fine
        struct RawBufferAllocatorNoFree
        {
            virtual void sub_0()
            {
                // Do nothing, we don't know what this does...
            }

            virtual void Free(void* aPtr)
            {
                constexpr auto shouldLogFreeCalls = false;

                if constexpr (shouldLogFreeCalls)
                {
                    PluginContext::Spew(std::format("RawBufferAllocatorNoFree::Free called, buf: {:#x}",
                                                    reinterpret_cast<uintptr_t>(aPtr)));
                }
                // Do nothing, again...
                // We clean up the memory anyway
            }

            virtual Red::Memory::IAllocator* GetAllocator()
            {
                return Red::Memory::RTTIAllocator::Get();
            }

            std::uint64_t m_unknown{};
        };

        auto bufPointer = reinterpret_cast<Red::DataBuffer*>(aOut);

        const auto bufferSize = aCursor.readUInt();

        if (bufferSize >= 0x80000000 || !bufferSize)
        {
            return;
        }

        auto& rawBuf = bufPointer->buffer;

        new (rawBuf.allocator) RawBufferAllocatorNoFree();
        
        rawBuf.data = aCursor.GetCurrentPtr();
        rawBuf.size = bufferSize;

        aCursor.offset += bufferSize;
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
        {
            const auto propSize = aPropType->GetSize();

            switch (propSize)
            {
            case 1u:
                *reinterpret_cast<std::int8_t*>(aOut) = aCursor.readByte();
                break;
            case 2u:
                *reinterpret_cast<std::int16_t*>(aOut) = aCursor.readShort();
                break;
            case 4u:
                *reinterpret_cast<std::int32_t*>(aOut) = aCursor.readInt();
                break;
            case 8u:
                *reinterpret_cast<std::int64_t*>(aOut) = aCursor.readInt64();
                break;
            default:
                aCursor.CopyTo(aOut, propSize);
                break;
            }

            return true;
        }
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
            else if (typeName == Red::CName{"DataBuffer"})
            {
                ReadDataBuffer(aCursor, aOut);
                // AAAAAAAAAAAAAAAAAAAA
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