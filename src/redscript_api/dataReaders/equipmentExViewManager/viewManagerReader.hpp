#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace ViewManagerReader
{
class ViewManagerResults : public Red::IScriptable
{
public:
    std::uint64_t m_source{};

	ViewManagerResults() = default;
    ViewManagerResults(Red::Handle<Red::ISerializable>* aViewManager) noexcept;

	RTTI_IMPL_TYPEINFO(ViewManagerResults);
    RTTI_IMPL_ALLOCATOR();
};
}

RTTI_DEFINE_CLASS(ViewManagerReader::ViewManagerResults, { RTTI_GETTER(m_source); });

