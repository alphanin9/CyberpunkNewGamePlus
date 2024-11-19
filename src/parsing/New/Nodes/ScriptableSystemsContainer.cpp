#include <Shared/Util/BufferCreator.hpp>
#include <Shared/Util/NamePoolRegistrar.hpp>

#include "ScriptableSystemsContainer.hpp"

using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool parser::node::ScriptableSystemsContainerNode::OnRead(Save::Stream::LoadStream& aStream) noexcept
{
    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (aStream.IsGood())
    {
        auto buf = aStream.ReadBuffer();

        m_reader.Init(std::move(buf));

        m_reader.m_readCruids = true;

        m_reader.ReadPackage();

        return aStream.IsGood();
    }

    return false;
}

CName parser::node::ScriptableSystemsContainerNode::GetName() noexcept
{
    return NamePoolRegistrar<"ScriptableSystemsContainer">::Get();
}