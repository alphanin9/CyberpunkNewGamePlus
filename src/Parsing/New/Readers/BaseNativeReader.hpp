#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "BufferCursor.hpp"

namespace parser::reader
{
class BaseNativeReader
{
protected:
    bool ReadProperty(Red::CBaseRTTIType* aType, void* aInstance, Cursor& aCursor) noexcept;
    bool ReadArray(Red::CRTTIBaseArrayType* aType, void* aInstance, Cursor& aCursor) noexcept;
    bool ReadWeakHandle(Red::WeakHandle<Red::ISerializable>& aHandle, Cursor& aCursor) noexcept;

    void ReadEnum(Red::CEnum* aType, void* aInstance, Cursor& aCursor) noexcept;
    void ReadDataBuffer(Red::DataBuffer& aBuffer, Cursor& aCursor) noexcept;
    void ReadTDBID(Red::TweakDBID& aId, Cursor& aCursor) noexcept;
    void ReadCName(Red::CName& aName, Cursor& aCursor) noexcept;
    void ReadNodeRef(Red::NodeRef& aNodeRef, Cursor& aCursor) noexcept;
public:
    bool ReadHandle(Red::Handle<Red::ISerializable>& aHandle, Cursor& aCursor) noexcept;
    bool ReadClass(Red::CClass* aType, void* aInstance, Cursor& aCursor) noexcept;
};
}