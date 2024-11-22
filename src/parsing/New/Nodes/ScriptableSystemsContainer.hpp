#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>
#include <parsing/New/TypeDefinitions/NGPlusSaveNode.hpp>

#include <tsl/hopscotch_map.h>

#include <Shared/Raw/Package/ScriptableSystemsPackage.hpp>
#include <Shared/Raw/Save/Save.hpp>

namespace parser::node
{
class ScriptableSystemsContainerNode : public SaveNodeData
{
private:
    Red::RawBuffer m_buffer{};
    Red::PackageHeader m_packageHeader{};
    shared::raw::ScriptablePackage::ScriptablePackageExtractor m_packageExtractor{};

    tsl::hopscotch_map<Red::CClass*, std::uint32_t> m_systemIndexMap{};
    tsl::hopscotch_map<Red::CClass*, Red::Handle<Red::IScriptable>> m_handleCache{};
public:
    bool OnRead(shared::raw::Save::Stream::LoadStream& aStream) noexcept override;
    Red::CName GetName() noexcept override;

    Red::Handle<Red::IScriptable>& GetScriptableSystem(Red::CClass* aClass) noexcept;

    RTTI_IMPL_TYPEINFO(ScriptableSystemsContainerNode);
    RTTI_IMPL_ALLOCATOR();
};
}