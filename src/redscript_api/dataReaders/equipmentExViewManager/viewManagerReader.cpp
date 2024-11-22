#include "viewManagerReader.hpp"

#include <Shared/RTTI/PropertyAccessor.hpp>

using namespace Red;

ViewManagerReader::ViewManagerResults::ViewManagerResults(Red::Handle<Red::ISerializable>* aViewManager) noexcept
{
    auto& viewState = shared::rtti::GetClassProperty<Handle<IScriptable>, "state">(*aViewManager);

    if (!viewState)
    {
        return;
    }

    auto itemSourceProp = viewState->GetType()->GetProperty("itemSource");
    auto itemSourceType = reinterpret_cast<CEnum*>(itemSourceProp->type);
    
    // Very hacky, but whatever
    itemSourceType->Assign(&m_source, itemSourceProp->GetValuePtr<void>(viewState));
}