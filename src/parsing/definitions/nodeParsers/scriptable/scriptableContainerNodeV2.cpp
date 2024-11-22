#include "scriptableContainerNodeV2.hpp"

using namespace Red;

void modsave::ScriptableSystemsContainerNodeV2::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto dataSize = static_cast<std::uint32_t>(aCursor.readInt());

    m_package.Init(aCursor.CreateSubCursor(dataSize));

    m_package.m_readCruids = true;
    m_package.m_useRootClassOptimization = false;

    m_package.ReadPackage();
}

Handle<ISerializable>* modsave::ScriptableSystemsContainerNodeV2::GetScriptableSystem(CName aName) noexcept
{
    return m_package.GetChunkByTypeName(aName);
}