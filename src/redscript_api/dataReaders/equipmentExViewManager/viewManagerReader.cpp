#include "viewManagerReader.hpp"

#include <../package/packageReader.hpp>

using namespace Red;

ViewManagerReader::ViewManagerResults::ViewManagerResults(Red::Handle<Red::ISerializable>* aViewManager) noexcept
{
    auto& instance = *aViewManager;

    auto viewState = instance->GetType()->GetProperty("state")->GetValuePtr<Handle<IScriptable>>(instance.instance);

    if (!viewState)
    {
        return;
    }

    auto& viewStateInstance = *viewState;

    auto itemSourceProp = viewStateInstance->GetType()->GetProperty("itemSource");
    auto itemSourceType = reinterpret_cast<CEnum*>(itemSourceProp->type);
    
    // Very hacky, but whatever
    itemSourceType->Assign(&m_source, itemSourceProp->GetValuePtr<void>(viewStateInstance.instance));
}