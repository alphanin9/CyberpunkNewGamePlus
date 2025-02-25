#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace PlayerDevelopmentSystemReader
{
class PlayerDevelopmentSystemResults : public Red::IScriptable
{
public:
    // Some of those are better off being read from stats, I think?
    int m_perkPoints{};
    int m_relicPoints{};
    int m_attributePoints{};

    PlayerDevelopmentSystemResults() = default;
    PlayerDevelopmentSystemResults(Red::Handle<Red::ISerializable>* aPlayerDevelopmentSystem) noexcept;

    RTTI_IMPL_TYPEINFO(PlayerDevelopmentSystemResults);
    RTTI_IMPL_ALLOCATOR();
};
}

RTTI_DEFINE_CLASS(PlayerDevelopmentSystemReader::PlayerDevelopmentSystemResults, {
    RTTI_GETTER(m_perkPoints);
    RTTI_GETTER(m_relicPoints);
    RTTI_GETTER(m_attributePoints);
});