#include "MacSessionDesc.hpp"

#include <RedLib.hpp>
#include <Shared/Raw/Ink/InkSystem.hpp>

namespace ngp::mac
{
namespace
{
Red::CBaseRTTIType* FindType()
{
    auto* type = Red::CRTTISystem::Get()->GetType(Red::game::GameSessionDesc::NAME);
    if (!type || type->GetSize() != SessionDesc::Size || type->GetAlignment() > 16)
    {
        return nullptr;
    }
    return type;
}

template<typename T>
T& At(std::uint8_t* aStorage, std::size_t aOffset)
{
    return *reinterpret_cast<T*>(aStorage + aOffset);
}
}

SessionDesc::SessionDesc()
    : m_type(FindType())
    , m_constructed(false)
    , m_storage{}
    , worldId(At<Red::world::WorldID>(m_storage, WorldIdOffset))
    , mainQuests(At<Red::DynArray<Red::game::MainQuestData>>(m_storage, MainQuestsOffset))
    , characterCustomizationState(
          At<Red::Handle<Red::game::ui::CharacterCustomizationState>>(m_storage, CharacterCustomizationStateOffset))
{
    if (m_type)
    {
        m_type->Construct(m_storage);
        m_constructed = true;
    }
}

SessionDesc::~SessionDesc()
{
    if (m_constructed)
    {
        m_type->Destruct(m_storage);
    }
}

SessionDesc::operator bool() const
{
    return m_constructed;
}

void SessionDesc::AddTo(shared::raw::Ink::SessionData::Data& aSessionData, Red::CName aName)
{
    if (!m_constructed)
    {
        return;
    }
    shared::raw::Ink::SessionData::Data::AddArgumentInternal(aSessionData.m_arguments, aName, m_type, m_storage);
}
}
