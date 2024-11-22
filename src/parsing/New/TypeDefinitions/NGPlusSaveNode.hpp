#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <Shared/Raw/Save/Save.hpp>

namespace parser::node
{
class SaveNodeData : public Red::ISerializable
{
public:
    virtual bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept;
    virtual Red::CName GetName() noexcept;

    RTTI_IMPL_TYPEINFO(SaveNodeData);
    RTTI_IMPL_ALLOCATOR();
};
}