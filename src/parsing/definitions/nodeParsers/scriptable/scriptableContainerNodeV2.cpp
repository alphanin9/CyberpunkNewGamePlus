#include "scriptableContainerNodeV2.hpp"

void save::ScriptableSystemsContainerNodeV2::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto dataSize = static_cast<std::uint32_t>(aCursor.readInt());

    m_package.Init(aCursor.CreateSubCursor(dataSize));

    m_package.m_readCruids = true;
    m_package.m_useRootClassOptimization = false;

    m_package.ReadPackage();
}

Red::Handle<Red::ISerializable>* save::ScriptableSystemsContainerNodeV2::GetScriptableSystem(Red::CName aName) noexcept
{
    return m_package.GetChunkByTypeName(aName);
}