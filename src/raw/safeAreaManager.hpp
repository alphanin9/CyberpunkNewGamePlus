#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/AI/SafeAreaManager.hpp>
#include <RED4ext/Scripting/Natives/Vector4.hpp>

#include <detail/hashes.hpp>
#include <util/core.hpp>

namespace raw::SafeAreaManager
{
constexpr auto IsPointInSafeArea = util::RawFunc<detail::Hashes::SafeAreaManager_IsPointInSafeArea,
                                                 bool (*)(Red::AI::SafeAreaManager*, Red::Vector4&)>();
}