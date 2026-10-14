#pragma once

// Complete the generated types Handle<T> destroys. entIPlacedComponent.hpp only
// forward-declares ent::ITransformBinding, and Handle's destructor needs a complete
// ISerializable (is_base_of, then Delete). These headers live in the Mac NG+ build.
// The shared SDK checkout is not edited.
#include <RED4ext/Scripting/Natives/Generated/ent/ISourceBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
// SharedPunk's InkSystem destroys handles to these ink types.
#include <RED4ext/Scripting/Natives/Generated/ink/ControllerProcessor.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/VirtualWindow.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryItemInstance.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/anim/Processor.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/MultiChildren.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/PropertyManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/UserData.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IWidgetController.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/SpawningProcessor.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/StyleResourceWrapper.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLogicController.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryResource.hpp>
