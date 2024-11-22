#include <Shared/Util/NamePoolRegistrar.hpp>

#include "NGPlusSaveNode.hpp"

using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool parser::node::SaveNodeData::OnRead(Save::Stream::LoadStream& aStream) noexcept
{
    return false;
}

CName parser::node::SaveNodeData::GetName() noexcept
{
    return NamePoolRegistrar<"<unknown>">::Get();
}

RTTI_DEFINE_CLASS(parser::node::SaveNodeData, { RTTI_ABSTRACT(); });
