#pragma once
#include <Shared/RTTI/PropertyAccessor.hpp>
#include <Shared/Raw/Assert/AssertionFailed.hpp>
#include <Shared/Util/BufferCreator.hpp>

#include "RTTIReader.hpp"

using namespace parser::reader;
using namespace Red;

bool BaseNativeReader::ReadProperty(CBaseRTTIType* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    const auto typeId = aType->GetType();
    const auto typeName = aType->GetName();

    switch (typeId)
    {
    case ERTTIType::Name:
        ReadCName(*reinterpret_cast<CName*>(aInstance), aCursor);
        break;
    case ERTTIType::Fundamental:
        aCursor.CopyTo(aInstance, aType->GetSize());
        break;
    case ERTTIType::Class:
        return ReadClass(static_cast<CClass*>(aType), aInstance, aCursor);
    case ERTTIType::Array:
    case ERTTIType::StaticArray:
    case ERTTIType::NativeArray:
        return ReadArray(static_cast<CRTTIBaseArrayType*>(aType), aInstance, aCursor);
    case ERTTIType::Simple:
        if (typeName == GetTypeName<TweakDBID>())
        {
            ReadTDBID(*reinterpret_cast<TweakDBID*>(aInstance), aCursor);
        }
        else if (typeName == GetTypeName<NodeRef>())
        {
            ReadNodeRef(*reinterpret_cast<NodeRef*>(aInstance), aCursor);
        }
        else if (typeName == GetTypeName<DataBuffer>())
        {
            ReadDataBuffer(*reinterpret_cast<DataBuffer*>(aInstance), aCursor);
        }
        break;
    case ERTTIType::Enum:
        ReadEnum(static_cast<CEnum*>(aType), aInstance, aCursor);
        break;
    case ERTTIType::Handle:
        return ReadHandle(*reinterpret_cast<Handle<ISerializable>*>(aInstance), aCursor);
    case ERTTIType::WeakHandle:
        return ReadWeakHandle(*reinterpret_cast<WeakHandle<ISerializable>*>(aInstance), aCursor);
    default:
        return false;
    }

    return true;
}

bool BaseNativeReader::ReadArray(CRTTIBaseArrayType* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    const auto arraySize = aCursor.ReadPrimitive<std::uint32_t>();
    const auto innerType = aType->GetInnerType();

    aType->Resize(aInstance, arraySize);

    // Optimization: simple types can just be copied... (I think?)
    if (innerType->GetType() == Red::ERTTIType::Fundamental)
    {
        auto begin = aType->GetElement(aInstance, 0);
        aCursor.CopyTo(begin, arraySize * innerType->GetSize());

        return true;
    }

    for (auto i = 0u; i < arraySize; i++)
    {
        // Array elements are constructed on resize, we don't need to create a new instance
        auto elem = aType->GetElement(aInstance, i);
        if (!ReadProperty(innerType, elem, aCursor))
        {
            return false;
        }
    }

    return true;
}

bool BaseNativeReader::ReadWeakHandle(WeakHandle<ISerializable>& aHandle, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadWeakHandle is not valid");
    return false;
}

void BaseNativeReader::ReadEnum(CEnum* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadEnum is not valid");
}

void BaseNativeReader::ReadDataBuffer(Red::DataBuffer& aBuffer, Cursor& aCursor) noexcept
{
    const auto bufSize = aCursor.ReadPrimitive<std::uint32_t>();
    auto buf = shared::util::CreateBuffer(bufSize, 16u, Memory::RTTIAllocator::Get()); // RTTI allocator for RTTI stuff

    // F you
    // Unplacements your new
    new (&aBuffer.buffer) RawBuffer(std::move(buf));

    aCursor.CopyTo(aBuffer.buffer.data, bufSize);
}

void BaseNativeReader::ReadTDBID(Red::TweakDBID& aId, Cursor& aCursor) noexcept
{
    aId = aCursor.ReadPrimitive<TweakDBID>();
}

void BaseNativeReader::ReadCName(Red::CName& aName, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadCName is not valid");
}

void BaseNativeReader::ReadNodeRef(Red::NodeRef& aNodeRef, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadNodeRef is not valid");
}

bool BaseNativeReader::ReadHandle(Handle<ISerializable>& aHandle, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadHandle is not valid");
    return false;
}

bool BaseNativeReader::ReadClass(CClass* aType, ScriptInstance aInstance, Cursor& aCursor) noexcept
{
    shared::raw::Assert::RaiseAssert(std::source_location::current(), "BaseNativeReader::ReadClass is not valid");
    return false;
}