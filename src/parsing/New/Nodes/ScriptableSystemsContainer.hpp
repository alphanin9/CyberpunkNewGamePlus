#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <parsing/New/Readers/PackageReader.hpp>
#include <parsing/New/TypeDefinitions/NGPlusSaveNode.hpp>

namespace parser::node
{
class ScriptableSystemsContainerNode : public SaveNodeData
{
private:
    reader::PackageReader m_reader{};
public:
    bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept override;
    Red::CName GetName() noexcept override;
};
}