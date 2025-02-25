#include <algorithm>

#include <Shared/Util/BufferCreator.hpp>
#include <Shared/Util/NamePoolRegistrar.hpp>

#include "ScriptableSystemsContainer.hpp"

using namespace parser::node;
using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool ScriptableSystemsContainerNode::OnRead(Save::Stream::LoadStream& aStream) noexcept
{
    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (!aStream.IsGood())
    {
        return false;
    }

    auto buffer = aStream.ReadBuffer();

    // When you're too lazy to go and make a move ctor xD
    // Evil code
    // TODO: make operator, get it to R4e SDK
    std::copy_n(reinterpret_cast<char*>(&buffer), sizeof(RawBuffer), reinterpret_cast<char*>(&m_buffer));
    std::fill_n(reinterpret_cast<char*>(&buffer), sizeof(RawBuffer), 0);

    ScriptablePackage::ScriptablePackageReader reader(m_buffer);

    // Yes, the game does this
    reader.ReadHeader(m_packageHeader);
    reader.ReadHeader(m_packageHeader);

    ScriptablePackage::ScriptablePackageExtractor::InitFromHeader(&m_packageExtractor, m_packageHeader);

    for (auto i = 0u; i < reader.rootChunkTypes.size; i++)
    {
        m_systemIndexMap.insert_or_assign(GetClass(reader.rootChunkTypes[i]), i);
    }

    return aStream.IsGood();
}

CName ScriptableSystemsContainerNode::GetName() noexcept
{
    return NamePoolRegistrar<"ScriptableSystemsContainer">::Get();
}

Handle<IScriptable>& ScriptableSystemsContainerNode::GetScriptableSystem(CClass* aClass) noexcept
{
    static Handle<IScriptable> s_empty;

    if (m_handleCache.contains(aClass))
    {
        return m_handleCache[aClass];
    }

    if (!m_systemIndexMap.contains(aClass))
    {
        return s_empty;
    }

    m_handleCache.insert_or_assign(aClass, MakeScriptedHandle<IScriptable>(aClass));

    auto index = m_systemIndexMap[aClass];

    m_packageExtractor.GetObjectById(m_handleCache[aClass], index);

    return m_handleCache[aClass];
}

RTTI_DEFINE_CLASS(parser::node::ScriptableSystemsContainerNode, { RTTI_PARENT(parser::node::SaveNodeData); });